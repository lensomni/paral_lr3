#pragma once

#include <sys/shm.h>
#include <sys/ipc.h>
#include <unistd.h>
#include <cstdlib>
#include <sys/types.h>
#include <sys/msg.h>
#include <stdio.h>
#include <fcntl.h>     
#include <sys/file.h>  
#include <string>

#include "common.h"   

struct SharedState {
    int releaseStage;              // арбитр повышает, когда разрешает следующий этап
    int arrivedStage[MAX_CARS];    // машина пишет номер этапа, который завершила
};

class Barrier {
private:
    int shmid;
    SharedState* status;
    int car_count;
    int msgid;

public:
    Barrier(int cars, key_t msg_key);
    ~Barrier();

    void waitStageStart(int stage);               
    void markArrived(int car_id, int stage);     
    void waitForRelease(int stage);   

    bool allCarsArrived(int stage) const;        
    void releaseNextStage(int stage);       

    int  getMsgQueueId() const;
    SharedState* getSharedState() const; 
};