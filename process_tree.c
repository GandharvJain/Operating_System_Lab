#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

void printPID(){
	printf("Child pid:%d with parent pid:%d\n", getpid(), getppid());
}

int main(int argc, char const *argv[]) {
	printPID();

	int p2 = fork(), p3 = 0, p21 = 0, p22 = 0, p31 = 0, p32 = 0;
	if (p2 == 0) {				/*Child process p2*/
		printPID();

		p21 = fork();

		if (p21 == 0) {					/*Child process p21*/
			printPID();
		}
		else if (p21 > 0) {				/*Parent process p2*/
			p22 = fork();

			if (p22 == 0) {				/*Child process p22*/
				printPID();
			}
		}
	}
	else if (p2 > 0) {			/*Parent process p1*/
		p3 = fork();

		if (p3 == 0) {			/*Child process p3*/
			printPID();
			p31 = fork();

			if (p31 == 0) {				/*Child process p31*/
				printPID();
			}
			else if (p31 > 0) {			/*Parent process p3*/
				p32 = fork();

				if (p32 == 0) {			/*Child process p32*/
					printPID();
				}
			}
		}
	}
	sleep(1);
	if (p32 || p22) {
		if (p32)
			printf("After killing:\n");
		exit(0);
	}
	sleep(1);
	printPID();
	return 0;
}
// "gcc process_tree.c -o process_tree.o; ./process_tree.o && sleep 2" to compile and run
// "pstree -phsTa $$" to get ancestors