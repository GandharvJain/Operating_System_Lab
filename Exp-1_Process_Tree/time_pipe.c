/*
Programming practice 3.19 in Operating Systems Concepts Silberschatz
Using Pipes
Compile using:
gcc time_pipe.c -o time_pipe.o
Run using:
./time_pipe.o [command]
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/time.h>
#include <sys/wait.h>

int main(int argc, char const *argv[]) {
	if (argc < 2) {
		printf("Usage: %s [command]\n", argv[0]);
		return 1;
	}

	char **args = malloc(argc * sizeof(char *));
	for (int i = 1; i < argc; ++i) {
		args[i - 1] = strdup(argv[i]);
	}
	args[argc - 1] = NULL;

	int SIZE = sizeof(struct timeval);
	struct timeval *start = malloc(SIZE);
	struct timeval *end = malloc(SIZE);

	int fd[2];
	pipe(fd);

	int pid = fork();
	if (pid < 0) {
		printf("Could not fork\n");
	}
	else if (pid == 0) {
		close(fd[0]);

		gettimeofday(start, 0);

		write(fd[1], start, SIZE);
		close(fd[1]);

		execvp(argv[1], args);
		printf("Couldn't execute %s\n", argv[1]);
		exit(1);
	}
	else {
		close(fd[1]);
		read(fd[0], start, SIZE);
		close(fd[0]);

		wait(0);

		gettimeofday(end, 0);

		double secnds = end->tv_sec - start->tv_sec;
		double micro_secnds = end->tv_usec - start->tv_usec;
		double runtime = secnds + micro_secnds / 1000000;

		printf("Time taken by '%s': %f s\n", argv[1], runtime);

		free(start);
		free(end);
		for (int i = 0; i < argc - 1; ++i)
			free(args[i]);
		free(args);
	}

	return 0;
}