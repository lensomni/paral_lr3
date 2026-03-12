#include <mpi.h>
#include <iostream>
using namespace std;

class Matrix {
	public:
	// Вывод матрицы
	static void printMatrix(int* matrix, int n, int m, bool invert = false) {
		if (invert) {
			for(int i = 0; i < n; i++ ) {
				for(int j = 0;  j < m;  j++ ) {
					cout << matrix[j * n + i] << "  \t";
				}
				cout << endl;
			}
		} else {
			for(int i = 0; i < n; i++ ) {
				for(int j = 0;  j < m;  j++ ) {
					cout << matrix[i * m + j] << "  \t";
				}
				cout << endl;
			}
		}
	}

	// Заполнение
	static void initialize(int* matrix, int n, int m) {
		// srand(time(0));
		for(int i = 0; i < n; i++ ) {
			for(int j = 0;  j < m;  j++ ) {
				matrix[i * m + j] = rand() % 10;
				cout << matrix[i * m + j] << "     ";
			}
			cout << endl;
		}
	}

	static int* createMatrix(int n, int m)
	{
		int *matrix;
		matrix = new int [n * m];
		initialize(matrix, n, m);
		return matrix;
	}

	// Транспонирование
	static void transponse(int* arr, int n, int m) {
		if(n == m){
			for(int r = 0; r < n; ++r){
				for(int c = r; c < m; ++c)
					swap(arr[r*m + c], arr[c*m + r]);
			}
		} else {
			int* tmp = new int[n * m];
			for(int r = 0; r < n; ++r){
				for(int c = 0; c < m; ++c)
					tmp[c*n + r] = arr[r*m + c];
			}
			memcpy(arr, tmp, (size_t)(n * m) * sizeof(int));
			delete[] tmp;
		}
	}
};