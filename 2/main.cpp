#include "common.h"
#include "barrier.h"
#include "car.h"
#include "results.h"
#include <iostream>
#include <unistd.h>
#include <sys/wait.h>
#include <ctime>
#include <cstdlib>
#include <limits>
#include <vector>
#include <fcntl.h>
#include <sys/file.h>
#include <errno.h>
#include <cstring>   // для memset

int main() {
    setlocale(LC_ALL, "ru_RU.UTF-8");
    srand(time(nullptr));
    key_t msg_key = ftok("/tmp", 'B');
    Barrier barrier(MAX_CARS, msg_key);
    int msgid = barrier.getMsgQueueId();

    // Создаём и блокируем файлы для этапов
    std::vector<int> stage_fds(STAGES, -1);
    for (int s = 1; s <= STAGES; ++s) {
        std::string path = "/tmp/stage" + std::to_string(s) + ".lock";
        int fd = open(path.c_str(), O_RDWR | O_CREAT, 0666);
        if (fd < 0) {
            perror("open lock");
            exit(1);
        }
        if (flock(fd, LOCK_EX) == -1) {
            perror("flock EX");
            exit(1);
        }
        stage_fds[s-1] = fd;
    }

    pid_t pids[MAX_CARS];
    for (int i = 0; i < MAX_CARS; ++i) {
        pids[i] = fork();
        if (pids[i] == 0) {
            Car car(i, barrier);
            car.race();
            exit(0);
        } else if (pids[i] < 0) {
            perror("fork");
            exit(1);
        }
    }

    // Инициализация общих результатов
    CarTotalResult total_results[MAX_CARS];
    for (int i = 0; i < MAX_CARS; ++i) {
        total_results[i].car_id = i + 1;
        total_results[i].total_points = 0;
        // обнуляем stage_results
        for (int s = 0; s < STAGES; ++s) {
            total_results[i].stage_results[s].place = 0;
            total_results[i].stage_results[s].points = 0;
        }
    }

    int stage_places[STAGES][MAX_CARS] = {0};

    for (int stage = 1; stage <= STAGES; ++stage) {
        std::cout << "\033c";
        std::cout << "\n=== ЭТАП " << stage << " ===\n";

        // Разблокируем старт этапа
        int idx = stage - 1;
        if (flock(stage_fds[idx], LOCK_UN) == -1) {
            perror("flock UN in main");
        }
        close(stage_fds[idx]);
        stage_fds[idx] = -1;

        // Сброс для этапа
        int positions[MAX_CARS] = {0};
        int finished_count = 0;
        int next_place = 1;  // места начинаются с 1

        while (finished_count < MAX_CARS) {
            Message msg;
            while (msgrcv(msgid, &msg, sizeof(Message) - sizeof(long), 0, IPC_NOWAIT) != -1) {
                if (msg.mtype == MSG_PROGRESS) {
                    positions[msg.car_id] = msg.current_distance;
                }
                else if (msg.mtype == MSG_FINISH_STAGE) {
                    int car = msg.car_id;
                    if (car >= 0 && car < MAX_CARS && stage_places[stage-1][car] == 0) {
                        stage_places[stage-1][car] = next_place++;
                        finished_count++;
                    }
                }
            }
            if (errno == ENOMSG) errno = 0;

            // Отрисовка текущего состояния
            std::cout << "\033c";
            std::cout << "\n=== ЭТАП " << stage << " ===\n";
            for (int i = 0; i < MAX_CARS; ++i) {
                int progress = (positions[i] * 100) / DISTANCE;
                int pos = (progress * 50) / 100;
                int line = 5 + i * 3;
                std::cout << "\033[" << line     << ";0H\033[K\033[" << pos << "C    ______";
                std::cout << "\033[" << (line+1) << ";0H\033[K\033[" << pos << "C __/  __  \\__";
                std::cout << "\033[" << (line+2) << ";0H\033[K\033[" << pos << "C'---O----O----'" ;
            }
            fflush(stdout);

            usleep(30000); 
        }

        // Барьер: ждём, пока все отметились
        while (!barrier.allCarsArrived(stage)) {
            usleep(1000);
        }
        barrier.releaseNextStage(stage);

        // Начисляем очки по местам
        int points_table[6] = {0, 25, 18, 15, 12, 10};  // индекс = место

        for (int car = 0; car < MAX_CARS; ++car) {
            int place = stage_places[stage-1][car];
            if (place == 0) place = MAX_CARS;  // на случай ошибки

            int points = points_table[place];

            total_results[car].stage_results[stage-1].place = place;
            total_results[car].stage_results[stage-1].points = points;
            total_results[car].total_points += points;
        }

        // Выводим результаты этапа
        printResults(stage, total_results);

        if (stage < STAGES) {
            std::cout << "\033[" << (22 + 7 + MAX_CARS) << ";0H";
            std::cout << "Нажмите Enter, чтобы продолжить...";
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            std::cin.get();
        }
    }

    // Cleanup оставшихся lock-файлов (если есть)
    for (int fd : stage_fds) {
        if (fd != -1) {
            flock(fd, LOCK_UN);
            close(fd);
        }
    }

    // Ждём завершения всех машин
    for (int i = 0; i < MAX_CARS; ++i) {
        waitpid(pids[i], nullptr, 0);
    }

    return 0;
}