#include <stdio.h>
#include <unistd.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>

#define USE_READLINE 0
#define CMD_MAX 1024
#define PWD_MAX 1024
#define MAX_ARGS (CMD_MAX/2)

#if USE_READLINE
#include <readline/readline.h>
#include <readline/history.h>
#endif

#define RED     "\x1b[31m"
#define GREEN   "\x1b[32m"
#define YELLOW  "\x1b[33m"
#define BLUE    "\x1b[34m"
#define MAGENTA "\x1b[35m"
#define CYAN    "\x1b[36m"
#define WHITE   "\x1b[37m"
#define RESET   "\x1b[0m"
#define BRIGHT  "\x1b[1m"

char* promptString(){
	char user[LOGIN_NAME_MAX], host[HOST_NAME_MAX], cwd[PWD_MAX];
	char *home = getenv("HOME");
	gethostname(host, sizeof(host));
	getlogin_r(user, sizeof(user));
	getcwd(cwd, sizeof(cwd));
	if (strstr(cwd, home) == cwd) {
		memmove(&cwd[1], &cwd[strlen(home)], strlen(cwd) - strlen(home) + 1);
		cwd[0] = '~';
	}
	char* str = (char*)malloc(7*8 + LOGIN_NAME_MAX + HOST_NAME_MAX + PWD_MAX + 5);
	sprintf(str, BRIGHT RED "%s@%s" RESET ":" BRIGHT CYAN "%s\n" BRIGHT YELLOW "$ ", user, host, cwd);
	return str;
}

void strDelim(char* cmd, char* delim, char **args) {
	args[0] = strtok(cmd, delim);
	for (int i = 1; i < MAX_ARGS && args[i-1]; ++i) {
		args[i] = strtok(NULL, delim);
	}
}

void parser(char* c) {
	char *args[MAX_ARGS], cmd[CMD_MAX];
	strcpy(cmd, c);
	strDelim(cmd, " ", args);

	printf("Arguments:\n");
	for (int i = 0; i < MAX_ARGS && args[i]; ++i) {
		printf("%s\n", args[i]);
	}
}

char* readCommand(){
	char *prompt = promptString();
	#if USE_READLINE
		char* c = readline(prompt);
		free(prompt);

		if (!c) {
			printf("\nExiting..\n");
			exit(0);
		}

		add_history(c);
		if (strlen(c) >= CMD_MAX) {
			printf("Argument list too long, input not executed\n");
			c = "";
		}
	#else
		printf("%s", prompt);
		free(prompt);

		char *c = (char*)malloc(CMD_MAX);
		if (!fgets(c, CMD_MAX, stdin)) {
			printf("\nExiting..\n");
			exit(0);
		}

		size_t len = strlen(c);
		if (c[len - 1] == '\n')
			c[len - 1] = '\0';
		else {
			while ((getchar()) != '\n');
			printf("Argument list too long, input dicarded\n");
			c[0] = '\0';
		}
	#endif
	char *cmd = malloc(strlen(c)+1);
	strcpy(cmd, c);
	free(c);
	return cmd;
}

void initText() {
	printf("\n-------Welcome to myShell-------\n"
		"Project created by Gandharv Jain\n");
	if (USE_READLINE)
		printf("    (Using readline library)    \n\n");
	else
		printf("(Without using readline library)\n\n");
}

void userMenu() {
	int running = 1;
	initText();

	while (running) {
		char* cmd = readCommand();

		if (strlen(cmd) == 0)
			continue;

		printf(RESET);

		parser(cmd);
		printf("Your command: \'%s\' of length %ld\n", cmd, strlen(cmd));

		free(cmd);
	}
}

int main(int argc, char const *argv[]) {
	userMenu();
	return 0;
}