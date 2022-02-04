#include <stdio.h>
#include <unistd.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/wait.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <errno.h>

#define CMD_MAX_LEN 1024
#define PWD_MAX_LEN 1024
#define MAX_ARGS ((int) CMD_MAX_LEN/2)
#define MAX_CMDS ((int) CMD_MAX_LEN/2)

#define SAVE_EMPTY_CMD 0
#define DEBUG_LEVEL 0

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

// Error checking
void DBG_checkClose(int e, char *str, char *mode, char *prntOrChld, char *cmd) {
	if (DEBUG_LEVEL > 1 && e) {
		printf("Error %d closing %s end of %s pipe in %s for process %s\n", errno, mode, str, prntOrChld, cmd);
	}
}
void DBG_checkDup(int e, char *Old, char *New, char *cmd) {
	if (DEBUG_LEVEL > 1 && e) {
		printf("Error %d duplicating %s fd to %s fd in process %s\n", errno, Old, New, cmd);
	}
}
void DBG_checkWait(int e) {
	if (DEBUG_LEVEL > 1 && e < 0) {
		printf("Error %d returned by wait\n", errno);
	}
}
void DBG_checkPipe(int e) {
	if (DEBUG_LEVEL > 1 && e) {
		printf("Error %d creating pipe\n", errno);
	}
}
void DBG_checkMalloc(char *e) {
	if (DEBUG_LEVEL > 1 && e == NULL) {
		printf("Error %d allocating memory\n", errno);
	}
}
void DBG_checkArgs(int argc, char **args) {
	if (DEBUG_LEVEL) {
		printf("Arguments (%d):\n", argc);
		for (int i = 0; i < MAX_ARGS && args[i]; ++i) {
		     printf("(%s)\n", args[i]);
		}
		printf("-----\n");
	}
}
void DBG_checkCmds(int num_cmds, char **cmds) {
	if (DEBUG_LEVEL) {
		printf("No. of Cmds = %d\n", num_cmds);
		for (int i = 0; i < num_cmds; ++i)
			printf("Cmd : (%s)\n", cmds[i]);
	}
}


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
	DBG_checkMalloc(str);

	sprintf(str, BRIGHT RED "%s@%s" RESET ":" BRIGHT CYAN "%s\n" BRIGHT YELLOW "$ ", user, host, cwd);
	return str;
}

void commandHandler(int argc, char **args, int* p1, int* p2) {
	int pid = fork();
	if (pid < 0)
		printf("Couldn't fork\n");
	else if (pid > 0) {
		if (p1[0] != -1)
			DBG_checkClose(close(p1[0]), " input", "r", "parent", args[0]);
		if (p1[1] != -1)
			DBG_checkClose(close(p1[1]), " input", "w", "parent", args[0]);
		DBG_checkWait(wait(0));
	}
	else {
		if (p1[1] != -1)
			DBG_checkClose(close(p1[1]), " input", "w", " child", args[0]);
		if (p1[0] != -1) {
			DBG_checkDup(dup2(p1[0], STDIN_FILENO), "r-in", "stdin", args[0]);
			DBG_checkClose(close(p1[0]), " input", "r", " child", args[0]);
		}

		if (p2[0] != -1)
			DBG_checkClose(close(p2[0]), "output", "r", " child", args[0]);
		if (p2[1] != -1) {
			DBG_checkDup(dup2(p2[1], STDOUT_FILENO), "w-out", "stdout", args[0]);
			DBG_checkClose(close(p2[1]), "output", "w", " child", args[0]);
		}

		if (execvp(args[0], args)) {
			printf("Couldn't execute %s\n", args[0]);
			exit(1);
		}
	}
}

int tokenise(char* cmd, char* delim, char **args, int ignoreQuotes) {
	int i = 0, n = strlen(delim), m = strlen(cmd);
	int foundDelim = 1;

	for (int j = 0; j < m; ++j) {

		while (strncmp(cmd + j, delim, n) == 0) {
		    cmd[j] = '\0';
		    j += n;
		    foundDelim = 1;
		}

		if (cmd[j] == '\"' || cmd[j] == '\'') {
			int s = j++;
			while (cmd[j] != cmd[s] && j < m) ++j;
			int e = j++;

			if (!ignoreQuotes) {
				cmd[s] = '\0';
				args[i++] = cmd + s + 1;
				cmd[e] = '\0';
			}
		}

		while (strncmp(cmd + j, delim, n) == 0) {
		    cmd[j] = '\0';
		    j += n;
		    foundDelim = 1;
		}

		if (foundDelim && j < m) {
			args[i++] = cmd + j;
			foundDelim = 0;
		}
	}
	args[i] = NULL;
	return i;
}

void initPipes(int n, int pipes[][2]) {
	for (int i = 1; i < n; ++i)
		DBG_checkPipe(pipe(pipes[i]));

	pipes[0][0] = pipes[0][1] = -1;
	pipes[n][0] = pipes[n][1] = -1;
}

void processCommand(char* c) {
	char cmd[CMD_MAX_LEN], *cmds[MAX_CMDS];
	strcpy(cmd, c);
	int num_cmds = tokenise(cmd, "|", cmds, 1);

	DBG_checkCmds(num_cmds, cmds);

	int pipes[num_cmds + 1][2];
	initPipes(num_cmds, pipes);

	for (int i = 0; i < num_cmds; ++i) {
		char *args[MAX_ARGS];
		int argc = tokenise(cmds[i], " ", args, 0);

		DBG_checkArgs(argc, args);

		commandHandler(argc, args, pipes[i], pipes[i+1]);
	}
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
		DBG_checkMalloc(c);

		if (fgets(c, CMD_MAX_LEN, stdin) == NULL)
			closeShell();

		size_t len = strlen(c);
		if (c[len - 1] == '\n') {
			c[len - 1] = '\0';
			c[len] = '\0';
		}
		else {
			while ((getchar()) != '\n');
			printf("Argument list too long, input dicarded\n");
			c[0] = '\0';
		}
	}
	if (strlen(c) || SAVE_EMPTY_CMD)
		addToHistory(c);

	char *cmd = malloc(strlen(c)+1);
	DBG_checkMalloc(cmd);

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



		// printf("Your command: \'%s\' of length %ld\n", cmd, strlen(cmd));