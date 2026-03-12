#include <mpi.h>
#include <iostream>
#include "matrix.cpp"

using namespace std;

void task2(int worldSize, int worldRank, int *A, int *B, int *C) {
    if(worldSize != 20 && worldRank == 0) {
        cout << "Неверно задано кол-во процессов\n\n";
        MPI_Abort(MPI_COMM_WORLD, -1);
        exit(0);
    }

    MPI_Comm mainProc;
    MPI_Comm subProc;
    MPI_Comm_split(MPI_COMM_WORLD, worldRank % 5, worldRank, &mainProc);
    MPI_Comm_split(MPI_COMM_WORLD, worldRank / 5, worldRank, &subProc);

    int elA;
    MPI_Scatter(A, 1, MPI_INT, &elA, 1, MPI_INT, 0, MPI_COMM_WORLD);
    for (int i = 0; i < 6; ++i) {
        int* colB = new int [5];
        if (worldRank % 5 == 0) {
            if (worldRank == 0) {
                memcpy(colB, B + i * 5, sizeof(int) * 5);
            }
            MPI_Bcast(colB, 5, MPI_INT, 0, mainProc);
        }
        int elB;
        MPI_Scatter(colB, 1, MPI_INT, &elB, 1, MPI_INT, 0, subProc);
        int res = elA * elB;
        int sum;
        MPI_Reduce(&res, &sum, 1, MPI_INT, MPI_SUM, 0, subProc);

        if (worldRank % 5 == 0) {
            MPI_Gather(&sum, 1, MPI_INT, C + i * 4, 1, MPI_INT, 0, mainProc);
        }
    }
}
