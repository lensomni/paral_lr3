#include "barrier.h"

Barrier::Barrier(int cars, key_t msg_key) : car_count(cars) {
    shmid = shmget(IPC_PRIVATE, sizeof(SharedState), 0666 | IPC_CREAT);
    if (shmid == -1) {
        perror("shmget");
        exit(1);
    }

    status = (SharedState*) shmat(shmid, nullptr, 0);
    if (status == (SharedState*)-1) {
        perror("shmat");
        exit(1);
    }

    status->releaseStage = 0;
    for (int i = 0; i < MAX_CARS; ++i) {
        status->arrivedStage[i] = 0;
    }

    msgid = msgget(msg_key, IPC_CREAT | 0666);
    if (msgid == -1) {
        perror("msgget");
        exit(1);
    }
}

Barrier::~Barrier() {
    if (status != (SharedState*)-1) {
        shmdt(status);
    }
    if (shmid != -1) {
        shmctl(shmid, IPC_RMID, nullptr);
    }
    if (msgid != -1) {
        msgctl(msgid, IPC_RMID, nullptr);
    }
}

// Для машин — ожидание старта этапа через flock
void Barrier::waitStageStart(int stage) {
    std::string path = "/tmp/stage" + std::to_string(stage) + ".lock";

    int fd = open(path.c_str(), O_RDWR);
    if (fd == -1) {
        perror("open stage lock (car)");
        exit(1);
    }

    // Блокируемся, пока арбитр держит эксклюзивную блокировку
    if (flock(fd, LOCK_SH) == -1) {
        perror("flock LOCK_SH (car)");
        close(fd);
        exit(1);
    }

    // Как только арбитр отпустил — сразу выходим
    flock(fd, LOCK_UN);
    close(fd);
}

// Машина сообщает, что доехала до конца этапа
void Barrier::markArrived(int car_id, int stage) {
    if (car_id < 0 || car_id >= MAX_CARS) return;
    status->arrivedStage[car_id] = stage;
}

// Машина ждёт разрешения от арбитра на следующий этап
void Barrier::waitForRelease(int stage) {
    while (status->releaseStage < stage) {
        usleep(1000);
    }
}

// Арбитр проверяет, все ли машины завершили этап
bool Barrier::allCarsArrived(int stage) const {
    for (int i = 0; i < car_count; ++i) {
        if (status->arrivedStage[i] < stage) {
            return false;
        }
    }
    return true;
}

// Арбитр разрешает переход к следующему этапу
void Barrier::releaseNextStage(int stage) {
    status->releaseStage = stage;
}

int Barrier::getMsgQueueId() const {
    return msgid;
}

SharedState* Barrier::getSharedState() const {
    return status;
}