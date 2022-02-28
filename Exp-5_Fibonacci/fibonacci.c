/*
Programming practice 4.27 in Operating Systems Concepts Silberschatz (10th ed.)
Compile using:
gcc -pthread fibonacci.c -o fibonacci.o
Run using:
./fibonacci.o [integer]
*/

#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
typedef long long ll;

ll *fib = NULL;
void *fib_calc(void*);
void print_fib(int);

int main(int argc, char const *argv[]) {
	if (argc < 2) {
		printf("Usage: %s [integer]\n", argv[0]);
		return 1;
	}

	int n = atoi(argv[1]);
	if (n <= 0) {
		printf("Please enter a positive integer\n");
		return 1;
	}
	fib = malloc((n + 1) * sizeof(ll));

	pthread_t tid;
	pthread_create(&tid, NULL, fib_calc, &n);
	pthread_join(tid, NULL);

	print_fib(n);
	return 0;
}

void *fib_calc(void *terms) {
	int n = *((int *)terms);

	fib[0] = 0;
	fib[1] = 1;
	for (int i = 2; i < n; ++i) {
		fib[i] = fib[i - 1] + fib[i - 2];
	}

	pthread_exit(0);
}

void print_fib(int n) {
	printf("Fibonacci sequence (%d terms): \n", n);
	for (int i = 0; i < n; ++i)
		printf("%lld ", fib[i]);
	printf("\n");
}