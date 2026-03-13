#include "results.h"

// void processStageResults(int stage, Message results[MAX_CARS], CarTotalResult total_results[MAX_CARS]) {
//     for (int i = 0; i < MAX_CARS - 1; ++i) {
//         for (int j = 0; j < MAX_CARS - i - 1; ++j) {
//             if (results[j].finish_time_ms > results[j + 1].finish_time_ms) {
//                 std::swap(results[j], results[j + 1]);
//             }
//         }
//     }

//     int points_table[] = {25, 18, 15, 12, 10};
//     for (int i = 0; i < MAX_CARS; ++i) {
//         int place = i + 1;
//         int points = points_table[i];
//         total_results[results[i].car_id].stage_results[stage - 1] = {results[i].finish_time_ms, place, points};
//         total_results[results[i].car_id].total_points += points;
//     }
// }

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

    // Сортировка по сумме очков для итогового места
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

    // Вычисляем итоговые места по очкам
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