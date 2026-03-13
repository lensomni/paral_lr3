#pragma once
#include "barrier.h"
#include <sys/time.h>

class Car {
private:
    int id;
    Barrier& barrier;
    int speed;
public:
    Car(int id, Barrier& barrier);
    void race();
    void drive_stage(int stage);
};