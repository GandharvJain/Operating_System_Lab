// Programming practice 3.19 in Operating Systems Concepts Silberschatz
// Compile using:
// gcc time_shm.c -lrt -o time_shm.o
// Run using:
// ./time_shm.o [command]

#include <stdio.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>

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
	const char *name = "timeCMD";
	struct timeval *start, *end = malloc(SIZE);

	int fd = shm_open(name, O_CREAT | O_RDWR, 0666);
	ftruncate(fd, SIZE);
	start = (struct timeval*) mmap(0, SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

	int pid = fork();
	if (pid < 0) {
		printf("Could not fork\n");
	}
	else if (pid == 0) {
		gettimeofday(start, 0);
		execvp(argv[1], args);
		printf("Couldn't execute %s\n", argv[1]);
		exit(1);
	}
	else {
		wait(0);
		gettimeofday(end, 0);

		double secnds = end->tv_sec - start->tv_sec;
		double micro_secnds = end->tv_usec - start->tv_usec;
		double runtime = secnds + micro_secnds / 1000000;

		printf("Time taken by '%s': %f s\n", argv[1], runtime);


		shm_unlink(name);
		free(end);
		for (int i = 0; i < argc - 1; ++i)
			free(args[i]);
		free(args);
	}

	return 0;
}