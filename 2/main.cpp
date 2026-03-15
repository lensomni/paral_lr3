#include <mpi.h>
#include <iostream>
#include <iomanip>
#include <cstdlib>
#include <ctime>
#include <unistd.h>
#include <limits>
#include <string>

// mpic++ main.cpp -o paral3_2
// mpirun --oversubscribe -np 6 ./paral3_2

#define MAX_CARS    5
#define STAGES      3
#define DISTANCE    10000
#define TRACK_WIDTH 50 

struct CarStageResult {
    int place;
    int points;
};

struct CarTotalResult {
    int car_id;
    CarStageResult stage_results[STAGES];
    int total_points;
};

// Вывод итоговой таблицы (арбитр)
void printResults(int stage, CarTotalResult total_results[MAX_CARS]) {
    std::cout << "\n\n";
    std::cout << "| Машина |";
    for (int s = 1; s <= stage; ++s)
        std::cout << "     Этап " << s << "      |";
    std::cout << "      Итог       |\n";

    std::cout << "+--------+";
    for (int s = 1; s <= stage; ++s)
        std::cout << "-----------------+";
    std::cout << "-----------------+\n";

    CarTotalResult sorted[MAX_CARS];
    for (int i = 0; i < MAX_CARS; ++i) sorted[i] = total_results[i];
    for (int i = 0; i < MAX_CARS - 1; ++i)
        for (int j = 0; j < MAX_CARS - i - 1; ++j)
            if (sorted[j].total_points < sorted[j + 1].total_points)
                std::swap(sorted[j], sorted[j + 1]);

    int final_places[MAX_CARS];
    for (int i = 0; i < MAX_CARS; ++i)
        for (int j = 0; j < MAX_CARS; ++j)
            if (total_results[i].car_id == sorted[j].car_id)
                final_places[i] = j + 1;

    for (int i = 0; i < MAX_CARS; ++i) {
        std::cout << "|  " << std::setw(2) << total_results[i].car_id << "    |";
        for (int s = 0; s < stage; ++s) {
            std::cout << " " << std::setw(2) << total_results[i].stage_results[s].place
                      << " место | " << std::setw(2) << total_results[i].stage_results[s].points
                      << " б |";
        }
        std::cout << " " << std::setw(2) << final_places[i]
                  << " место |" << std::setw(3) << total_results[i].total_points << " б |\n";
    }
    std::cout << "\n\n";
}

void drawCars(int stage, int positions[MAX_CARS]) {
    std::cout << "\033c";
    std::cout << "=== ЭТАП " << stage << " / " << STAGES << " ===\n\n";

    const std::string top    = "    ______";
    const std::string middle = " __/  __  \\__";
    const std::string bottom = "'---O----O----'";
    const int max_pos = TRACK_WIDTH - (int)bottom.size() - 1;

    for (int i = 0; i < MAX_CARS; ++i) {
        int pos = ((positions[i] * 100) / DISTANCE) * max_pos / 100;
        std::string pad(pos, ' ');

        std::cout << pad << top    << "\n";
        std::cout << pad << middle << "\n";
        std::cout << pad << bottom << "\n";
    }
    fflush(stdout);
}

void init_total_results(CarTotalResult total_results[]) {
    for (int i = 0; i < MAX_CARS; ++i) {
        total_results[i].car_id = i + 1;
        total_results[i].total_points = 0;
        for (int s = 0; s < STAGES; ++s) {
            total_results[i].stage_results[s].place = 0;
            total_results[i].stage_results[s].points = 0;
        }
    }
}

// Пошаговое движение для отправки прогресса через MPI_Gather
void drive_tick(int& distance_covered, int seed_base, int car_id, int tick) {
    if (distance_covered >= DISTANCE) return;
    srand(seed_base + car_id * 1000 + tick * 37);
    int speed = 100 + (rand() % 81);
    distance_covered += speed;
    if (distance_covered > DISTANCE) distance_covered = DISTANCE;
    usleep(1000000 / speed);
}

int main(int argc, char** argv) {

    MPI_Init(&argc, &argv);

    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    if (size != MAX_CARS + 1) {
        if (rank == 0)
            std::cerr << "Запустите с " << (MAX_CARS + 1)
                      << " процессами:\n  mpirun -np "
                      << (MAX_CARS + 1) << " ./race\n";
        MPI_Finalize();
        return 1;
    }

    CarTotalResult total_results[MAX_CARS];
    if (rank == 0) {
        init_total_results(total_results);
    }

    for (int stage = 1; stage <= STAGES; ++stage) {
        // синхронизация старта этапа
        MPI_Barrier(MPI_COMM_WORLD);

        // MPI_Bcast — арбитр рассылает seed этапа
        int stage_seed = 0;
        if (rank == 0)
            stage_seed = (int)time(nullptr) + stage * 1000;
        MPI_Bcast(&stage_seed, 1, MPI_INT, 0, MPI_COMM_WORLD); // <<< MPI_Bcast: сигнал старта + seed

        int my_distance = 0;
        int my_done     = 0;

        int all_distances[MAX_CARS + 1] = {0};
        int all_done_buf[MAX_CARS + 1]  = {0};
        int positions[MAX_CARS]         = {0};
        int finished[MAX_CARS]          = {0};
        int finish_order[MAX_CARS]      = {0};
        int next_place     = 1;
        int finished_count = 0;

        int tick = 0;
        while (true) {

            if (rank > 0 && my_done == 0) {
                drive_tick(my_distance, stage_seed, rank - 1, tick);
                if (my_distance >= DISTANCE)
                    my_done = 1;
            }

            MPI_Gather(&my_distance, 1, MPI_INT, all_distances, 1, MPI_INT, 0, MPI_COMM_WORLD);
            MPI_Gather(&my_done, 1, MPI_INT, all_done_buf, 1, MPI_INT, 0, MPI_COMM_WORLD);

            int stop = 0;
            if (rank == 0) {
                for (int i = 0; i < MAX_CARS; ++i) {
                    positions[i] = all_distances[i + 1];
                    if (positions[i] >= DISTANCE && finished[i] == 0) {
                        finished[i] = next_place;
                        finish_order[next_place - 1] = i;
                        next_place++;
                        finished_count++;
                    }
                }
                drawCars(stage, positions);

                if (finished_count == MAX_CARS) stop = 1;
            }
            MPI_Bcast(&stop, 1, MPI_INT, 0, MPI_COMM_WORLD); //сигнал конца этапа
            if (stop) break;

            tick++;
        }

        if (rank == 0) {
            int points_table[6] = {0, 25, 18, 15, 12, 10};
            for (int place = 1; place <= MAX_CARS; ++place) {
                int car = finish_order[place - 1];
                total_results[car].stage_results[stage - 1].place  = place;
                total_results[car].stage_results[stage - 1].points = points_table[place];
                total_results[car].total_points += points_table[place];
            }
            printResults(stage, total_results);

            if (stage < STAGES) {
                std::cout << "Нажмите Enter для следующего этапа...";
                std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
                std::cin.get();
            }
        }
        MPI_Barrier(MPI_COMM_WORLD); // конец этапа
    }

    if (rank == 0)
        std::cout << "=== ГОНКА ЗАВЕРШЕНА ===\n";

    MPI_Finalize();
    return 0;
}