#pragma once
#include <sys/msg.h>
#include <sys/ipc.h>
#include <iostream>
#include <unistd.h>
#include <cstdlib>
#include <ctime>
#include <string> 

#define MAX_CARS 5
#define STAGES 3
#define DISTANCE 30000
#define MSG_PROGRESS 10
#define MSG_FINISH_STAGE 2

struct Message {
    long mtype;
    int car_id;
    int current_distance;
    //long finish_time_ms;
};