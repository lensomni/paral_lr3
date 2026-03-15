#include <mpi.h>
#include <iostream>
#include <cstdlib>
#include <ctime>
#include <unistd.h>
#include <iomanip>
#include <limits>
#include <vector>

#define MAX_CARS 5
#define STAGES 3
#define DISTANCE 5000

struct Message {
    int car_id;
    int current_distance;
};

struct StageResult {
    int car_id;
    int place;
};

struct CarStageResult {
    int place;
    int points;
};

struct CarTotalResult {
    int car_id;
    CarStageResult stage_results[STAGES];
    int total_points;
};

void printResults(int stage, CarTotalResult total_results[MAX_CARS]) {
    std::cout << "\033[22;0H";

    std::cout << "| Машина |";
    for (int s = 1; s <= stage; ++s) {
        std::cout << "      Этап " << s << "     |";
    }
    std::cout << "      Итог       |\n";

    std::cout << "+--------+";
    for (int s = 1; s <= stage; ++s) {
        std::cout << "-----------------+";
    }
    std::cout << "-----------------+\n";

    CarTotalResult sorted_results[MAX_CARS];
    for (int i = 0; i < MAX_CARS; ++i) {
        sorted_results[i] = total_results[i];
    }
    for (int i = 0; i < MAX_CARS - 1; ++i) {
        for (int j = 0; j < MAX_CARS - i - 1; ++j) {
            if (sorted_results[j].total_points < sorted_results[j + 1].total_points) {
                std::swap(sorted_results[j], sorted_results[j + 1]);
            }
        }
    }

    int final_places[MAX_CARS];
    for (int i = 0; i < MAX_CARS; ++i) {
        for (int j = 0; j < MAX_CARS; ++j) {
            if (total_results[i].car_id == sorted_results[j].car_id) {
                final_places[i] = j + 1;
                break;
            }
        }
    }

    for (int i = 0; i < MAX_CARS; ++i) {
        std::cout << "|   " << std::setw(2) << total_results[i].car_id << "   |";
        for (int s = 0; s < stage; ++s) {
            std::cout << " " << std::setw(2) << total_results[i].stage_results[s].place << " место"
                      << " | " << std::setw(2) << total_results[i].stage_results[s].points << " б |";
        }
        std::cout << " " << std::setw(2) << final_places[i] << " место"
                  << " |" << std::setw(3) << total_results[i].total_points << " б |\n";
    }
    std::cout << "\n\n";
}

void draw_progress(int stage, int positions[]) {
    std::cout << "\033c";
    std::cout << "\n=== ЭТАП " << stage << " ===\n";

    for (int i = 0; i < MAX_CARS; ++i) {
        int progress = (positions[i] * 100) / DISTANCE;
        int pos = (progress * 50) / 100;
        int line = 5 + i * 3;

        std::cout << "\033[" << line     << ";0H\033[K\033[" << pos << "C    ______";
        std::cout << "\033[" << (line+1) << ";0H\033[K\033[" << pos << "C __/  __  \\__";
        std::cout << "\033[" << (line+2) << ";0H\033[K\033[" << pos << "C'---O----O----'";
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

void update_stage_results(CarTotalResult total_results[], int stage_places[], int stage) {
    int points_table[6] = {0, 25, 18, 15, 12, 10};

    for (int car = 0; car < MAX_CARS; ++car) {
        int place = stage_places[car];
        if (place == 0) place = MAX_CARS;

        int points = points_table[place];

        total_results[car].stage_results[stage-1].place = place;
        total_results[car].stage_results[stage-1].points = points;
        total_results[car].total_points += points;
    }
}

void wait_for_enter(int stage) {
    if (stage < STAGES) {
        std::cout << "\033[" << (22 + 7 + MAX_CARS) << ";0H";
        std::cout << "Нажмите Enter, чтобы продолжить...";
        std::string dummy;
        std::getline(std::cin, dummy);
    }
}

void send_message(int car_id, int distance, int type) {
    Message msg;
    msg.car_id = car_id;
    msg.current_distance = distance;
    MPI_Send(&msg, sizeof(Message), MPI_BYTE, 0, type, MPI_COMM_WORLD);
}

int main(int argc, char *argv[]) {
    MPI_Init(&argc, &argv);

    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    if (size != MAX_CARS + 1) {
        if (rank == 0) {
            std::cerr << "Неверное количество процессов. Ожидается " << MAX_CARS + 1 << " процессов." << std::endl;
        }
        MPI_Finalize();
        return 1;
    }

    if (rank == 0) {
        CarTotalResult total_results[MAX_CARS];
        init_total_results(total_results);

        for (int stage = 1; stage <= STAGES; ++stage) {
            int start_signal;
            MPI_Bcast(&start_signal, 1, MPI_INT, 0, MPI_COMM_WORLD);

            std::cout << "\033c";
            std::cout << "\n=== ЭТАП " << stage << " ===\n";

            int positions[MAX_CARS] = {0};
            bool stage_finished = false;
            int finished_count = 0;
            int stage_places[MAX_CARS] = {0};
            int next_place = 1;
            bool finished[MAX_CARS] = {false};

            while (!stage_finished) {
                MPI_Status status;
                Message msg;
                int flag;

                MPI_Iprobe(MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &flag, &status);
                if (flag) {
                    MPI_Recv(&msg, sizeof(Message), MPI_BYTE, status.MPI_SOURCE,
                            status.MPI_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

                    switch (status.MPI_TAG) {
                        case 0: // о прогрессе
                            if (!finished[msg.car_id]) {
                                positions[msg.car_id] = msg.current_distance;
                            }
                            break;

                        case 1: // о завершении этапа
                            if (stage_places[msg.car_id] == 0) {
                                stage_places[msg.car_id] = next_place++;
                                finished_count++;
                            }
                            finished[msg.car_id] = true;
                            break;
                    }
                }
                draw_progress(stage, positions);

                if (finished_count == MAX_CARS) { stage_finished = true; }
                usleep(20000);
            }

            StageResult all_results[MAX_CARS];
            MPI_Gather(MPI_IN_PLACE, 0, MPI_DATATYPE_NULL, all_results,
                    sizeof(StageResult), MPI_BYTE, 0, MPI_COMM_WORLD);

            update_stage_results(total_results, stage_places, stage);
            printResults(stage, total_results);
            wait_for_enter(stage);
        }
    }
    else {
        int car_id = rank - 1;
        srand(time(nullptr) + car_id);

        for (int stage = 1; stage <= STAGES; ++stage) {
            int distance_covered = 0;
            int speed;

            while (distance_covered < DISTANCE) {
                speed = 200 + (rand() % 81);
                distance_covered += speed;
                if (distance_covered > DISTANCE) distance_covered = DISTANCE;

                send_message(car_id, distance_covered, 0);  // 0 - прогресс
                usleep(1000000 / speed);
            }
            send_message(car_id, distance_covered, 1);  // 1 - финиш

            StageResult local_result;
            local_result.car_id = car_id;
            local_result.place = 0;

            MPI_Gather(&local_result, sizeof(StageResult), MPI_BYTE, nullptr,
                       sizeof(StageResult), MPI_BYTE, 0, MPI_COMM_WORLD);
        }
    }

    MPI_Finalize();
    return 0;
}
