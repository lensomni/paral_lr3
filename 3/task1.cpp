#include <mpi.h>
#include <iostream>
#include "matrix.cpp"

using namespace std;

void task1(int worldSize, int worldRank, int *A, int *B, int *C) {
    if(worldSize != 4 && worldRank == 0) {
        cout << "Неверно задано кол-во процессов\n\n";
        MPI_Abort(MPI_COMM_WORLD, -1);
        exit(0);
    }

    int rowA[5];
    MPI_Scatter(A, 5, MPI_INT, &rowA, 5, MPI_INT, 0, MPI_COMM_WORLD);

    for (int c = 0; c < 6; ++c) {
        int colB[5];
        if (worldRank == 0) {
            memcpy(colB, B + c * 5, sizeof(int) * 5);
        }
        MPI_Bcast(colB, 5, MPI_INT, 0, MPI_COMM_WORLD);

        int res = 0;
        for (int i = 0; i < 5; ++i) {
            res += rowA[i] * colB[i];
        }

        MPI_Gather(&res, 1, MPI_INT, C + c * 4, 1, MPI_INT, 0, MPI_COMM_WORLD);
    }
}
