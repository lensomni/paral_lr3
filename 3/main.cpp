#include <mpi.h>
#include <ctime>
#include <iostream>
#include "matrix.cpp"
#include "task1.cpp"
#include "task2.cpp"

using namespace std;

// https://ru.onlinemschool.com/math/assistance/matrix/multiply/

//mpic++ main.cpp -o ./paral3_3
//mpirun --oversubscribe -np 4 ./paral3_3 1
//mpirun --oversubscribe -np 20 ./paral3_3 2


int main(int argc, char *argv[]) {
	MPI_Init(NULL, NULL);
	int *A, *B, *C;

	int worldSize, worldRank, nameLen;
	MPI_Comm_size(MPI_COMM_WORLD, &worldSize);
	MPI_Comm_rank(MPI_COMM_WORLD, &worldRank);

	if (worldRank == 0) {
		cout << "Mатрица A:\n";
		A = Matrix::createMatrix(4, 5);

		cout << "Матрица B:\n";
		B = Matrix::createMatrix(5, 6);
		Matrix::transponse(B, 5, 6);

		C = new int [4 * 6];
	}
	
	int time = clock();
	switch (*argv[1]) {
		case '1':
			task1(worldSize, worldRank, A, B, C);
			break;
		case '2':
			task2(worldSize, worldRank, A, B, C);
			break;
		default:
			if (worldRank == 0) {
				cout << "Неверно задан параметр - " << argv[1] << endl;
				MPI_Abort(MPI_COMM_WORLD, -1);
			}
			exit(0);
	}

	int timeRes = (float)(clock() - time) * 1000000 / CLOCKS_PER_SEC;

	if (worldRank == 0) {
		cout << "Матрица C:\n";
		Matrix::printMatrix(C, 4, 6, true);
		cout << "Время выполнения: " << timeRes << " микросекунд\n";
	}

	MPI_Finalize();
	return 0;
}