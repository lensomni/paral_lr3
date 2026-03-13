#include "car.h"
#include "common.h"
#include <sys/time.h>
#include <iostream>

Car::Car(int id, Barrier& barrier) : id(id), barrier(barrier) {
    srand(time(nullptr) + id);
}

void Car::drive_stage(int stage) {
    barrier.waitStageStart(stage);

    int distance_covered = 0;
    while (distance_covered < DISTANCE) {
        speed = 100 + (rand() % 81);
        distance_covered += speed;
        if (distance_covered > DISTANCE) distance_covered = DISTANCE;

        Message msg;
        msg.mtype = MSG_PROGRESS;
        msg.car_id = id;
        msg.current_distance = distance_covered;
        if (msgsnd(barrier.getMsgQueueId(), &msg, sizeof(Message) - sizeof(long), 0) == -1) {
            perror("msgsnd progress");
        }

        usleep(1000000 / speed);
    }

    Message finish_msg;
    finish_msg.mtype = MSG_FINISH_STAGE;
    finish_msg.car_id = id;
    finish_msg.current_distance = DISTANCE;

    if (msgsnd(barrier.getMsgQueueId(), &finish_msg, sizeof(Message) - sizeof(long), 0) == -1) {
        perror("msgsnd finish");
    }

    barrier.markArrived(id, stage);
    barrier.waitForRelease(stage);
}

void Car::race() {
    for (int stage = 1; stage <= STAGES; ++stage) {
        drive_stage(stage);
    }
}