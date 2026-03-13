#pragma once
#include "common.h"
#include <iomanip>
#include <iostream>

struct CarStageResult {
    //long finish_time_ms;
    int place;
    int points;
};

struct CarTotalResult {
    int car_id;
    CarStageResult stage_results[STAGES];
    int total_points;
};

//void processStageResults(int stage, Message results[MAX_CARS], CarTotalResult total_results[MAX_CARS]);
void printResults(int stage, CarTotalResult total_results[MAX_CARS]);
