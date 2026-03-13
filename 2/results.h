#pragma once
#include "common.h"
#include <iomanip>
#include <iostream>

struct CarStageResult {
    int place;
    int points;
};

struct CarTotalResult {
    int car_id;
    CarStageResult stage_results[STAGES];
    int total_points;
};

void printResults(int stage, CarTotalResult total_results[MAX_CARS]);
