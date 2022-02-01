#include <stdio.h>
#include <unistd.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>

#define MAX_LENGTH 256
#define CWD_MAX 1024

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

void userMenu() {
	int running = 1;
	while (running) {
	
		printPromptString();
		char cmd[MAX_LENGTH];

		if (!fgets(cmd, MAX_LENGTH, stdin))
			break;		//Exit if EOF

		size_t length = strlen(cmd);
		if (cmd[length - 1] == '\n') {cmd[length - 1] = '\0';}

		printf("Your command: %s\n", cmd);
	}
}

int main(int argc, char const *argv[]) {
	userMenu();
	return 0;
}