/*
Programming practice 3.21 in Operating Systems Concepts Silberschatz (10th ed.)
Compile using:
gcc collatz.c -o collatz.o
Run using:
./collatz.o [positive integer]
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

int main(int argc, char const *argv[]) {
	if (argc < 2) {
		printf("Usage: %s [positive integer]\n", argv[0]);
		return 1;
	}

	long long n = atoi(argv[1]);
	if (n < 1) {
		printf("Enter a positive integer\n");
		return 1;
	}

	int pid = fork();
	if (pid < 0) {
		printf("Could not fork\n");
	}
	else if (pid == 0) {
		while (n > 1) {
			printf("%lld, ", n);
		    if (n % 2)
		    	n = 3 * n + 1;
		    else
		    	n /= 2;
		}
		printf("%lld\n", n);
		return 0;
	}
	else if (pid > 0) {
		wait(0);
	}
	return 0;
}