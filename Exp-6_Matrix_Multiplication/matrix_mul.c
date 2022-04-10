/*
Compile using:
gcc -pthread matrix_mul.c -o matrix_mul.o
Run using:
./matrix_mul.o [row-1] [column-1] [row-2] [column-2]
*/

#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <sys/time.h>

#define NUM_THREADS 4
#define MAX (1<<8)

void init_Matrices(int, int, int, int);
void *mat_mul(void*);
void display(int, int, int, int);
void check(void*);

int **matrix_1, **matrix_2, **matrix_3;
typedef struct WorkRange {
	int start, end;
	int w1, w2;
} WorkRange;

int main(int argc, char const *argv[]) {
	if (argc < 5) {
		printf("Usage: %s [row-1] [column-1] [row-2] [column-2]\n", argv[0]);
		return 1;
	}
	int h1 = atoi(argv[1]), w1 = atoi(argv[2]);
	int h2 = atoi(argv[3]), w2 = atoi(argv[4]);
	if (h1 < 1 || w1 < 1 || h2 < 1 || w2 < 1) {
		printf("Please enter a positive integer\n");
		return 1;
	}
	if (w1 != h2) {
		printf("%d != %d, please enter valid dimensions\n", w1, h2);
		return 1;
	}

	init_Matrices(h1, w1, h2, w2);

	struct timeval start, end;
	gettimeofday(&start, 0);

	int workBlocks = h1 / NUM_THREADS;
	WorkRange w[NUM_THREADS];
	for (int i = 0; i < NUM_THREADS; ++i) {
		w[i].start = i * workBlocks;
		w[i].end = (i + 1) * workBlocks;
		w[i].w1 = w1;
		w[i].w2 = w2;
	}
	w[NUM_THREADS - 1].end = h1;

	pthread_t tid[NUM_THREADS];

	for (int i = 0; i < NUM_THREADS; ++i)
		pthread_create(&tid[i], NULL, mat_mul, &w[i]);

	for (int i = 0; i < NUM_THREADS; ++i)
		pthread_join(tid[i], NULL);

	gettimeofday(&end, 0);

	display(h1, w1, h2, w2);
	double secnds = end.tv_sec - start.tv_sec;
	double micro_secnds = end.tv_usec - start.tv_usec;
	double runtime = secnds + micro_secnds / 1000000;
	printf("\nTime taken: %f s\n", runtime);

	return 0;
}

void *mat_mul(void *arg) {
	WorkRange w = *((WorkRange *) arg);
	int s = w.start, e = w.end;
	int w1 = w.w1, w2 = w.w2;

	for (int i = s; i < e; ++i) {
		for (int j = 0; j < w2; ++j) {
			int temp = 0;
			for (int k = 0; k < w1; ++k) {
				temp += matrix_1[i][k] * matrix_2[k][j];
			}
			matrix_3[i][j] = temp;
		}
	}
}

void init_Matrices(int h1, int w1, int h2, int w2) {
	struct timeval tm;
	gettimeofday(&tm, 0);
	srand(tm.tv_sec * 1000 + tm.tv_usec / 1000);

	matrix_1 = malloc(h1 * sizeof(int *));
	check(matrix_1);

	for (int i = 0; i < h1; ++i) {
		matrix_1[i] = malloc(w1 * sizeof(int));
		check(matrix_1[i]);

		for (int j = 0; j < w1; ++j)
			matrix_1[i][j] = rand() % MAX;
	}

	matrix_2 = malloc(h2 * sizeof(int *));
	check(matrix_2);

	for (int i = 0; i < h2; ++i) {
		matrix_2[i] = malloc(w2 * sizeof(int));
		check(matrix_2[i]);

		for (int j = 0; j < w2; ++j)
			matrix_2[i][j] = rand() % MAX;
	}

	matrix_3 = malloc(h1 * sizeof(int *));
	check(matrix_3);

	for (int i = 0; i < h1; ++i) {
		matrix_3[i] = malloc(w2 * sizeof(int));
		check(matrix_3[i]);

		for (int j = 0; j < w2; ++j)
			matrix_3[i][j] = 0;
	}
}

void display(int h1, int w1, int h2, int w2) {
	printf("\nMatrix-1:\n");
	for (int i = 0; i < h1; ++i) {
		for (int j = 0; j < w1; ++j) {
			printf("%d ", matrix_1[i][j]);
		}
		printf("\n");
	}
	printf("\nMatrix-2:\n");
	for (int i = 0; i < h2; ++i) {
		for (int j = 0; j < w2; ++j) {
			printf("%d ", matrix_2[i][j]);
		}
		printf("\n");
	}
	printf("\nMatrix-3:\n");
	for (int i = 0; i < h1; ++i) {
		for (int j = 0; j < w2; ++j) {
			printf("%d ", matrix_3[i][j]);
		}
		printf("\n");
	}
}

void check(void* ptr) {
	if (ptr == NULL) {
		printf("Error allocating memory\n");
		exit(1);
	}
}
