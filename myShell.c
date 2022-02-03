#include <stdio.h>
#include <unistd.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/wait.h>
#include <readline/readline.h>
#include <readline/history.h>

#define CMD_MAX_LEN 1024
#define PWD_MAX_LEN 1024
#define MAX_ARGS ((int) CMD_MAX_LEN/2)
#define MAX_CMDS ((int) CMD_MAX_LEN/2)

#define SAVE_EMPTY_CMD 0

#define RED     "\x1b[31m"
#define GREEN   "\x1b[32m"
#define YELLOW  "\x1b[33m"
#define BLUE    "\x1b[34m"
#define MAGENTA "\x1b[35m"
#define CYAN    "\x1b[36m"
#define WHITE   "\x1b[37m"
#define RESET   "\x1b[0m"
#define BRIGHT  "\x1b[1m"

FILE *hist_file;
int use_readline = 0;
char last_cmd[CMD_MAX_LEN];

void closeShell() {
	if (hist_file)
		fclose(hist_file);
	printf("\nExiting..\n");
	exit(0);
}

char* promptString(){
	char user[LOGIN_NAME_MAX], host[HOST_NAME_MAX], cwd[PWD_MAX_LEN];
	char *home = getenv("HOME");
	gethostname(host, sizeof(host));
	getlogin_r(user, sizeof(user));
	getcwd(cwd, sizeof(cwd));
	if (strstr(cwd, home) == cwd) {
		memmove(&cwd[1], &cwd[strlen(home)], strlen(cwd) - strlen(home) + 1);
		cwd[0] = '~';
	}
	char* str = (char*)malloc(7*8 + LOGIN_NAME_MAX + HOST_NAME_MAX + PWD_MAX_LEN + 5);
	sprintf(str, BRIGHT RED "%s@%s" RESET ":" BRIGHT CYAN "%s\n" BRIGHT YELLOW "$ ", user, host, cwd);
	return str;
}

void commandHandler(int argc, char **args, int* p1, int* p2) {
	int pid = fork();
	if (pid < 0)
		printf("Couldn't fork\n");
	else if (pid > 0) {
		close(p1[0]);
		close(p1[1]);
		wait(0);
	}
	else {
		close(p1[1]);
		dup2(p1[0], STDIN_FILENO);
		close(p1[0]);

		close(p2[0]);
		dup2(p2[1], STDOUT_FILENO);
		close(p2[1]);

		if (execvp(args[0], args)) {
			printf("Couldn't execute %s\n", args[0]);
			exit(1);
		}
	}
}

int tokenise(char* cmd, char* delim, char **args) {
	args[0] = strtok(cmd, delim);
	int i = 1;
	while (i < MAX_ARGS && args[i-1])
		args[i++] = strtok(NULL, delim);
	return i - 1;
}

void processCommand(char* c) {
	char cmd[CMD_MAX_LEN], *cmds[MAX_CMDS];
	strcpy(cmd, c);
	int num_cmds = tokenise(cmd, "|", cmds);
	printf("%d\n", num_cmds);

	int pipes[num_cmds + 1][2];
	for (int i = 1; i < num_cmds; ++i)
		pipe(pipes[i]);

	pipes[num_cmds][0] = pipes[0][1] = -1;
	dup2(STDIN_FILENO, pipes[0][0]);
	dup2(STDOUT_FILENO, pipes[num_cmds][1]);


	for (int i = 0; i < num_cmds; ++i) {
		char *args[MAX_ARGS];
		int argc = tokenise(cmds[i], " ", args);

		printf("Arguments:\n");
		for (int i = 0; i < MAX_ARGS && args[i]; ++i) {
		     printf("%s\n", args[i]);
		}
		printf("---\n");

		commandHandler(argc, args, pipes[i], pipes[i+1]);
	}

	int wpid, status = 0;

	// for (int i = 1; i < num_cmds; ++i) {
	// 	close(pipes[i][0]);
	// 	close(pipes[i][1]);
	// }

	// while ((wpid = wait(&status)) > 0);

}

void addToHistory(char* command) {
	if (use_readline){
		add_history(command);
		append_history(1, ".myShell_history");
	}
	else if (hist_file) {
		// fprintf(hist_file, "#%ld\n%s\n", time(0), command);
		fprintf(hist_file, "%s\n", command);
		fflush(hist_file);
		strcpy(last_cmd, command);
	}
}

char* readCommand(){
	char *prompt = promptString(), *c;
	if (use_readline) {
		c = readline(prompt);
		free(prompt);

		if (c == NULL)
			closeShell();

		if (strlen(c) >= CMD_MAX_LEN) {
			printf("Argument list too long, input discarded\n");
			c = "";
		}
	}
	else {
		printf("%s", prompt);
		free(prompt);

		c = (char*)malloc(CMD_MAX_LEN);
		if (fgets(c, CMD_MAX_LEN, stdin) == NULL)
			closeShell();

		size_t len = strlen(c);
		if (c[len - 1] == '\n')
			c[len - 1] = '\0';
		else {
			while ((getchar()) != '\n');
			printf("Argument list too long, input dicarded\n");
			c[0] = '\0';
		}
	}
	if (strlen(c) || SAVE_EMPTY_CMD)
		addToHistory(c);

	char *cmd = malloc(strlen(c)+1);
	strcpy(cmd, c);
	free(c);
	return cmd;
}

void init() {
	printf("\n-------Welcome to myShell-------\n"
		"Project created by Gandharv Jain\n");
	if (use_readline)
		printf("    (Using readline library)    \n\n");
	else
		printf("(Without using readline library)\n\n");
	fflush(stdout);

	if (use_readline)
		read_history_range(".myShell_history", 0, -1);
	else  if (!( hist_file = fopen(".myShell_history", "a") ))
		printf("Warning! Could not load \".myShell_history\", histroy will not be saved!\n");
}

void myShell() {
	int running = 1;
	init();

	while (running) {
		char* cmd = readCommand();

		if (strlen(cmd) == 0)
			continue;

		printf(RESET);
		fflush(stdout);

		processCommand(cmd);

		printf("Your command: \'%s\' of length %ld\n", cmd, strlen(cmd));

		free(cmd);
	}
}

int main(int argc, char const *argv[]) {
	if (argc > 1)
		use_readline = atoi(argv[1]);
	myShell();
	return 0;
}

// main -> myShell --(init)-> readCommand --(promptString, addToHistory)-> processCommand --(tokenise)-> commandHandler