/*
Programming practice 3.21 in Operating Systems Concepts Silberschatz (10th ed.)
[Modified to use threads instead]
Compile using:
gcc -pthread collatz.c -o collatz.o
Run using:
./collatz.o [positive integer]
*/

#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>

#define MAX_ARR_LEN 1024

void *collatz_conj(void*);
void print_collatz(int*);

int main(int argc, char const *argv[]) {
	if (argc < 2) {
		printf("Usage: %s [terms]\n", argv[0]);
		return 1;
	}
	int n = atoi(argv[1]);
	if (n <= 0) {
		printf("Please enter a positive integer\n");
		return 1;
	}

	int *arr;
	pthread_t tid;
	pthread_create(&tid, NULL, collatz_conj, &n);
	pthread_join(tid, (void*)&arr);

	print_collatz(arr);
	return 0;
}

void *collatz_conj(void *start) {
	int n = *((int *)start);
	int *arr = malloc(MAX_ARR_LEN * sizeof(int));

	int i = 0;
	while (n > 1) {
		if (i >= MAX_ARR_LEN - 3) {
			printf("Sequence is too big\n");
			pthread_exit(0);
		}
		arr[i++] = n;
	    if (n % 2)
	    	n = 3 * n + 1;
	    else
	    	n /= 2;
	}
	arr[i++] = 1;
	arr[i] = -1;
	pthread_exit((void*)arr);
}

void print_collatz(int *arr) {
	printf("Collatz sequence:\n");
	for (int i = 0; arr[i] != -1; ++i)
		printf("%d ", arr[i]);
	printf("\n");
}