#include <stdio.h>
#include <unistd.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/wait.h>
#include <signal.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <errno.h>

#define CMD_MAX_LEN 1024
#define PWD_MAX_LEN 1024
#define MAX_ARGS ((int) CMD_MAX_LEN/2)
#define MAX_CMDS ((int) CMD_MAX_LEN/2)
#define MAX_STATEMENTS ((int) CMD_MAX_LEN/2)

#define SAVE_EMPTY_CMD 0	//0 for saving empty commands disabled
#define DEBUG_LEVEL 2		//0 for none, 1 for minimal, 2 for full

#define RED     "\x1b[31m"
#define GREEN   "\x1b[32m"
#define YELLOW  "\x1b[33m"
#define BLUE    "\x1b[34m"
#define MAGENTA "\x1b[35m"
#define CYAN    "\x1b[36m"
#define WHITE   "\x1b[37m"
#define RESET   "\x1b[0m"
/**********************************************************************************
*********************************Error Checking************************************
**********************************************************************************/
#define BRIGHT  "\x1b[1m"

/**********************************************************************************
*********************************Error Checking************************************
**********************************************************************************/

void DBG_checkClose(int e, char *str, char *mode, char *prntOrChld, char *cmd) {
	if (DEBUG_LEVEL > 1 && e) {
		printf("Error %d (%s) closing %s end of %s pipe in %s for process %s\n", errno, strerror(errno), mode, str, prntOrChld, cmd);
	}
}
void DBG_checkDup(int e, char *Old, char *New, char *cmd) {
	if (DEBUG_LEVEL > 1 && e) {
		printf("Error %d (%s) duplicating %s fd to %s fd in process %s\n", errno, strerror(errno), Old, New, cmd);
	}
}
void DBG_checkWait(int e) {
	if (DEBUG_LEVEL > 1 && e < 0) {
		printf("Error %d (%s) returned by wait\n", errno, strerror(errno));
	}
}
void DBG_checkPipe(int e) {
	if (DEBUG_LEVEL > 1 && e) {
		printf("Error %d (%s) creating pipe\n", errno, strerror(errno));
	}
}
void DBG_checkMalloc(char *e) {
	if (DEBUG_LEVEL > 1 && e == NULL) {
		printf("Error %d (%s) allocating memory\n", errno, strerror(errno));
	}
}
void DBG_checkFreopen(FILE *e, char *filename) {
	if (DEBUG_LEVEL > 1 && e == NULL) {
		printf("Error %d (%s) reopening file %s\n", errno, strerror(errno), filename);
	}
}
void DBG_checkArgs(int argc, char **args) {
	if (DEBUG_LEVEL) {
		printf("Arguments (%d):\n", argc);
		for (int i = 0; i < argc; ++i) {
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
void DBG_checkStatements(int num_statements, char **statements) {
	if (DEBUG_LEVEL) {
		printf("Statements (%d):\n", num_statements);
		for (int i = 0; i < num_statements; ++i) {
		     printf("(%s)\n", statements[i]);
		}
		printf("-----------------\n");
	}
}

/**********************************************************************************
*********************************myShell functions*********************************
**********************************************************************************/

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
	char* str = malloc(7*8 + LOGIN_NAME_MAX + HOST_NAME_MAX + PWD_MAX_LEN + 5);
	DBG_checkMalloc(str);

	sprintf(str, BRIGHT RED "%s@%s" RESET ":" BRIGHT CYAN "%s\n" BRIGHT YELLOW "$ ", user, host, cwd);
	return str;
}

void handleRedirect(int argc, char **args, int i) {
	char *mode = "r";
	FILE *stream = stdout;

	for (; i < argc; ++i) {
		if (strcmp(args[i], "<") == 0)
			stream = stdin;
		else if (strcmp(args[i], ">>") == 0)
			mode = "a";
		else if (strcmp(args[i], ">") == 0)
			mode = "w";
		else
			continue;

		break;
	}

	if (i >= argc)
		return;

	args[i++] = NULL;
	char *filename = args[i];
	args[i++] = NULL;

	DBG_checkFreopen(freopen(filename, mode, stream), filename);

	handleRedirect(argc, args, i);
}

int commandExec(int argc, char **args, int* p_in, int* p_out) {

	//Check for built in command---------------------------------------------------------------------------------------

	int pid = fork();
	if (pid < 0)			//Error
		printf("Couldn't fork\n");
	else if (pid > 0) {		//Parent process
		if (p_in[0] != -1)
			DBG_checkClose(close(p_in[0]), " input", "r", "parent", args[0]);
		if (p_in[1] != -1)
			DBG_checkClose(close(p_in[1]), " input", "w", "parent", args[0]);

		DBG_checkWait(wait(0));
	}
	else {					//Child process
		if (p_in[1] != -1)
			DBG_checkClose(close(p_in[1]), " input", "w", " child", args[0]);
		if (p_out[0] != -1)
			DBG_checkClose(close(p_out[0]), "output", "r", " child", args[0]);

		if (p_in[0] != -1) {
			DBG_checkDup(dup2(p_in[0], STDIN_FILENO), "r-in", "stdin", args[0]);
			DBG_checkClose(close(p_in[0]), " input", "r", " child", args[0]);
		}
		if (p_out[1] != -1) {
			DBG_checkDup(dup2(p_out[1], STDOUT_FILENO), "w-out", "stdout", args[0]);
			DBG_checkClose(close(p_out[1]), "output", "w", " child", args[0]);
		}

		handleRedirect(argc, args, 0);

		if (execvp(args[0], args)) {
			printf("Couldn't execute %s\n", args[0]);
			exit(1);
		}
	}
}

int tokenise(char* cmd, char* delim, char **args, int quotesToArg) {
	int i = 0, n = strlen(delim), m = strlen(cmd);
	int foundDelim = 1;

	for (int j = 0; j < m; ++j) {

		while (strncmp(cmd + j, delim, n) == 0) {
		    cmd[j] = '\0';
		    j += n;
		    foundDelim = 1;
		}

		if (j < m && strchr("\"\'", cmd[j])) {
			int s = j++;
			while (cmd[j] != cmd[s] && j < m) ++j;
			int e = j++;

			if (quotesToArg) {
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

void processCommand(char *c) {
	char str[CMD_MAX_LEN], *cmds[MAX_CMDS];
	strcpy(str, c);

	// Match continous or single only?---------------------------------------------------------------------------------
	int num_cmds = tokenise(str, "|", cmds, 0);

	DBG_checkCmds(num_cmds, cmds);

	int pipes[num_cmds + 1][2];
	initPipes(num_cmds, pipes);

	for (int i = 0; i < num_cmds; ++i) {
		char *args[MAX_ARGS];
		int argc = tokenise(cmds[i], " ", args, 1);

		DBG_checkArgs(argc, args);

		// Check return value for '&&' and '||' operators--------------------------------------------------------------

		commandExec(argc, args, pipes[i], pipes[i+1]);
	}
}

void statementsParser(char *c) {
	int last = strlen(c) - 1;
	int isBackground = 0;
	if (c[last] == '&' && c[last - 1] != '&') {
		c[last] = '\0';
		
		int pid = fork();
		if (pid < 0)
			printf("Couldn't create background process\n");
		else if (pid == 0) {
			printf("[%d] Started in background: %s\n", getpid(), c);
			fflush(stdout);

			statementsParser(c);

			printf("[%d] Done					%s\n", getpid(), c);
			exit(0);
		}
		else {
			signal(SIGCHLD, SIG_IGN);
			fflush(stdout);
			return;
		}
	}
	else {
		char str[CMD_MAX_LEN], *statements[MAX_STATEMENTS];
		strcpy(str, c);
		int num_statements = tokenise(str, ";", statements, 0);

		DBG_checkStatements(num_statements, statements);

		for (int i = 0; i < num_statements; ++i)
			processCommand(statements[i]);
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
	DBG_checkMalloc(cmd);

	strcpy(cmd, c);
	free(c);
	return cmd;
}

char* operatorSpacer(char *cmd) {
	char *delims[] = {">>", ">", "<"/*, "&&", "||"*/};
	int n = sizeof(delims) / sizeof(delims[0]), m = strlen(cmd);

	char *new_cmd = malloc(2 * strlen(cmd));
	DBG_checkMalloc(new_cmd);

	for (int i = 0, j = 0; i <= m;) {
		for (int k = 0; k < n; ++k) {
			char *d = delims[k];
			int d_len = strlen(d);

			if (strncmp(cmd + i, d, d_len) == 0 && i < m) {
				if (cmd[i - 1] != ' ')
					new_cmd[j++] = ' ';

				new_cmd[j] = '\0';

				strcat(new_cmd, d);
				j += d_len;
				i += d_len;

				if (cmd[i] != ' ')
					new_cmd[j++] = ' ';

				break;
			}
		}
		new_cmd[j++] = cmd[i++];
	}
	return new_cmd;
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

		if (strlen(cmd) == 0) {
			free(cmd);
			continue;
		}

		printf(RESET);
		fflush(stdout);

		char* spaced_cmd = operatorSpacer(cmd);
		free(cmd);

		statementsParser(spaced_cmd);
	}
}

int main(int argc, char const *argv[]) {
	if (argc > 1)
		use_readline = atoi(argv[1]);
	myShell();
	return 0;
}

// main -> myShell --(init)-> readCommand --(promptString, addToHistory)-> statementsParser -> 
// processCommand --(initPipes, tokenise)-> commandExec --(handleRedirect)-> 


		// printf("Your command: \'%s\' of length %ld\n", cmd, strlen(cmd));