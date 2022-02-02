#include <stdio.h>
#include <unistd.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>

#define MAX_LENGTH 1024
#define CWD_MAX 1024
#define MAX_ARGS (MAX_LENGTH/2)
void printPromptString(){
	char user[LOGIN_NAME_MAX], host[HOST_NAME_MAX], cwd[CWD_MAX];
	char *home = getenv("HOME");
	gethostname(host, sizeof(host));
	getlogin_r(user, sizeof(user));
	getcwd(cwd, sizeof(cwd));
	if (strstr(cwd, home) == cwd) {
		memmove(&cwd[1], &cwd[strlen(home)], strlen(cwd) - strlen(home) + 1);
		cwd[0] = '~';
	}
	printf("%s@%s: %s\n$ ", user, host, cwd);
}

void strDelim(char* cmd, char* delim, char **args) {
	args[0] = strtok(cmd, delim);
	for (int i = 1; i < MAX_ARGS && args[i-1]; ++i) {
		args[i] = strtok(NULL, delim);
	}
}

void parser(char* cmd) {
	char *args[MAX_ARGS];
	strDelim(cmd, " ", args);

	// printf("Arguments:\n");
	// for (int i = 0; i < MAX_ARGS && args[i]; ++i) {
	// 	printf("%s\n", args[i]);
	// }
}

void userMenu() {
	int running = 1;
	printf(	"\nWelcome to myShell!\nProject created by Gandharv Jain\n\n");
	
	while (running) {
		printPromptString();

		char cmd[MAX_LENGTH];
		if (!fgets(cmd, MAX_LENGTH, stdin))
			break;		//Exit if EOF

		size_t len = strlen(cmd);
		if (cmd[len - 1] == '\n') {
			cmd[len - 1] = '\0';
		}
		else {
			while ((getchar()) != '\n');
			printf("Argument list too long, input dicarded\n");
			continue;
		}

		parser(cmd);
		printf("Your command: %s of length %ld\n", cmd, strlen(cmd));
	}
}

int main(int argc, char const *argv[]) {
	userMenu();
	return 0;
}