#include <mpi.h>
#include <ctime>
#include <iostream>
#include "matrix.cpp"
using namespace std;

// https://ru.onlinemschool.com/math/assistance/matrix/multiply/

//mpic++ main.cpp -o ./paral3_3
//mpirun --oversubscribe -np 4 ./paral3_3 1
//mpirun --oversubscribe -np 20 ./paral3_3 2

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

		// cout << worldRank << "-ROW: " << rowA[0] << "-" << rowA[1] << "-" << rowA[2] << endl;
		// cout << worldRank << "-COL: " << colB[0] << "-" << colB[1] << "-" << colB[2] << endl;

		int res = 0;
		for (int i = 0; i < 5; ++i) {
			res += rowA[i] * colB[i];
		}

		MPI_Gather(&res, 1, MPI_INT, C + c * 4, 1, MPI_INT, 0, MPI_COMM_WORLD);
	}
}

void task2(int worldSize, int worldRank, int *A, int *B, int *C) {
	if(worldSize != 20 && worldRank == 0) {
		cout << "Неверно задано кол-во процессов\n\n";
		MPI_Abort(MPI_COMM_WORLD, -1);
		exit(0);
	}

	// 4 основных процесса, которые будут высчитывать сумму
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

int main(int argc, char *argv[]) {
	MPI_Init(NULL, NULL);
	int *A, *B, *C;

	int worldSize, worldRank, nameLen;
	MPI_Comm_size(MPI_COMM_WORLD, &worldSize);
	MPI_Comm_rank(MPI_COMM_WORLD, &worldRank);

	if (worldRank == 0) {
		cout << "Matrix A:\n";
		A = Matrix::createMatrix(4, 5);

		cout << "Matrix B:\n";
		B = Matrix::createMatrix(5, 6);
		Matrix::transponse(B, 5, 6);

		C = new int [4 * 6];
	}
	
	int time = clock();
	if (*argv[1] == '1') {
		task1(worldSize, worldRank, A, B, C);
	} else if (*argv[1] == '2') {
		task2(worldSize, worldRank, A, B, C);
	} else if (worldRank == 0) {
		cout << "Неверно задан праметр - " << argv[1] << endl;
		MPI_Abort(MPI_COMM_WORLD, -1);
		exit(0);
	}
	int timeRes = (float)(clock() - time) * 1000000 / CLOCKS_PER_SEC;

	if (worldRank == 0) {
		cout << "Matrix C:\n";
		Matrix::printMatrix(C, 4, 6, true);
		cout << "Время выполнения: " << timeRes << " микросекунд\n";
	}

	MPI_Finalize();
	return 0;
}