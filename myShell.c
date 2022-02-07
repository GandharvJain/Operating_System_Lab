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
#include <setjmp.h>

#define CMD_MAX_LEN 1024
#define PWD_MAX_LEN 1024
#define MAX_ARGS ((int) CMD_MAX_LEN/2)
#define MAX_CMDS ((int) CMD_MAX_LEN/2)
#define MAX_STATEMENTS ((int) CMD_MAX_LEN/2)

#define SAVE_EMPTY_CMD 0	//0 for saving empty commands disabled
#define DEBUG_LEVEL 0		//-1 for none, 0 for some, 1 for some more, 2 for full

#define RED     "\x1b[31m"
#define GREEN   "\x1b[32m"
#define YELLOW  "\x1b[33m"
#define BLUE    "\x1b[34m"
#define MAGENTA "\x1b[35m"
#define CYAN    "\x1b[36m"
#define WHITE   "\x1b[37m"
#define RESET   "\x1b[0m"
#define BRIGHT  "\x1b[1m"

/**********************************************************************************
*********************************Error Checking************************************
**********************************************************************************/

void DBG_checkChdir(int e) {
	if (DEBUG_LEVEL > -1 && e < 0) {
		printf("Error changing directory: %s\n", strerror(errno));
	}
}
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
********************************Function Prototypes********************************
**********************************************************************************/
void addProcess(int, int);
void killProcess(int, int);
void reset(int);

void updateLast();
void help(char*);
void source(char*);
void printHistory();

void closeShell(int);
char* promptString();
void handleRedirect(int, char**, int);
int builtInCmdExec(int, char**);
int commandExec(int, char**, int*, int*);
int tokenise(char*, char*, char**, int);
void initPipes(int, int[][2]);
void pipesParser(char*, int);
void statementsParser(char*);
void addToHistory(char*);
char* readCommand();
char* preprocessCmd(char*);
void init();
void myShell();

/*********************************************************************************
*********************************Global variables*********************************
*********************************************************************************/

FILE *hist_file;
int use_readline = 0;
char last_cmd[CMD_MAX_LEN] = "";
sigjmp_buf ctrlc_buf;

/**********************************************************************************
************************************Job Control************************************
**********************************************************************************/

typedef struct process {
	struct process *next;
	int pid;
	int isBg;
} process;

process *head = NULL;

void addProcess(int process_id, int isBackground) {
	if (process_id <= 0)
		return;
	process *p = malloc(sizeof(process));
	p->pid = process_id;
	p->isBg = isBackground;
	p->next = head;
	head = p;
}

void killProcess(int process_id, int killBg) {
	if (head == NULL) {
		return;
	}
	process *prev = head;
	if (process_id > 0) {
		kill(process_id, SIGKILL);

		if (head->pid == process_id) {
			if (head->isBg && !killBg)
				return;
			head = prev->next;
			free(prev);
			return;
		}
		for (process *curr = head->next; curr; curr = curr->next, prev = prev->next) {
			if (curr->pid == process_id) {
				if (curr->isBg && !killBg)
					return;
				prev->next = curr->next;
				free(curr);
				return;
			}
		}
	}
	else {
		process *curr = head;
		while (curr != NULL) {
			if (curr->isBg && !killBg) {
				curr = curr->next;
				continue;
			}
			kill(curr->pid, SIGKILL);
			prev = curr;
			curr = curr->next;
			free(prev);
		}
		head = NULL;
	}
}

/**********************************************************************************
**********************************Signal Handling**********************************
**********************************************************************************/

void reset(int sig) {
	printf("\n");
	fflush(stdout);
	killProcess(-1, 0);
	siglongjmp(ctrlc_buf, 1);
}

/**********************************************************************************
*********************************Built-in Commands*********************************
**********************************************************************************/

void updateLast() {
	FILE *hist = fopen(".myShell_history", "r");
	while (!feof(hist))
		fgets(last_cmd, CMD_MAX_LEN, hist);
	last_cmd[strlen(last_cmd) - 1] = '\0';
}

void help(char *cmd) {
	if (cmd == NULL) {
		printf(	"\nThese shell commands are defined internally.  Type 'help' to see this list.\n"
				"Type 'help name' to find out more about the function 'name'.\n"
				"Use 'man -k' or 'info' to find out more about commands not in this list.\n"
				"[Note: myShell replaces '!!' with the last command executed]\n\n"
				"\tcd [dir]\n\texit [n]\n\thelp [command]\n\thistory\n\t. filename\n\tsource filename\n\n");
	}
	else if (strcmp(cmd, "cd") == 0) {
		printf(	"cd: cd [dir]\n"
				"\tChange the shell working directory.\n\n"
				"\tChange the current directory to DIR.  The default DIR is the value of the\n"
				"\tHOME shell variable.\n\n");
	}
	else if (strcmp(cmd, "exit") == 0) {
		printf(	"exit: exit [n]\n"
			    "\tExit the shell.\n\n"
				"\tExits the shell with a status of N.  If N is omitted, the exit status is 0.\n\n");
	}
	else if (strcmp(cmd, "help") == 0) {
		printf(	"help: help [command]\n"
			    "\tDisplay information about builtin commands.\n\n"
				"\tDisplays brief summaries of builtin commands. If COMMAND isspecified,\n"
				"\tgives detailed help on command otherwise the list of help topics is printed.\n\n");
	}
	else if (strcmp(cmd, "history") == 0) {
		printf(	"history: history\n"
			    "\tDisplay the history list.\n\n"
				"\tDisplay the history list with line numbers.\n\n");
	}
	else if (strcmp(cmd, ".") == 0) {
		printf(	".: . filename\n"
			    "\tExecute commands from a file in the current shell.\n\n"
				"\tRead and execute commands from FILENAME in the current shell.\n"
				"\tFILENAME requires relative path or absolute path\n\n");
	}
	else if (strcmp(cmd, "source") == 0) {
		printf(	"source: source filename\n"
			    "\tExecute commands from a file in the current shell.\n\n"
				"\tRead and execute commands from FILENAME in the current shell.\n"
				"\tFILENAME requires relative path or absolute path\n\n");
	}
	else {
		printf("No such built-in command: '%s'\n\n", cmd);
	}
}

void source(char *filename) {
	FILE *fd;
	int line_num = 1;
	if( !(fd = fopen(filename, "r")) )
		printf("Couldn't source file %s\n", filename);
	else {
		char *cmd = malloc(CMD_MAX_LEN);
		DBG_checkMalloc(cmd);

		while(fgets(cmd, CMD_MAX_LEN, fd)) {
			size_t len = strlen(cmd);
			if (cmd[len - 1] == '\n')
				cmd[len - 1] = '\0';
			else {
				printf("Command too long, execution stopped\n");
				return;
			}

			statementsParser(preprocessCmd(cmd));
		}
	}
}

void printHistory() {
	char line[CMD_MAX_LEN] = "";
	int i = 1;
	rewind(hist_file);
	while (fgets(line, CMD_MAX_LEN, hist_file) != NULL) {
	    printf("\t%d\t%s", i, line);
	    ++i;
	}
}

/**********************************************************************************
*********************************myShell functions*********************************
**********************************************************************************/

void closeShell(int n) {
	printf("\nExiting..\n");
	if (hist_file)
		fclose(hist_file);
	killProcess(-1, 1);
	exit(n);
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

int builtInCmdExec(int argc, char **args) {
	char *builtInList[] = {"cd", "exit", "help", "history", ".", "source"};
	int i = 0, found = -1;
	while (i < 6) {
	    if (strcmp(args[0], builtInList[i]) == 0) {
	    	found = i;
	    	break;
	    }
	    ++i;
	}

	switch (found) {
		case 0: {
			char *dir = getenv("HOME");
			if (args[1])
				dir = args[1];
			DBG_checkChdir(chdir(dir));
			break;
		}
		case 1: {
			int n = 0;
			if (args[1])
				n = atoi(args[1]);
			closeShell(n);
			break;
		}
		case 2: {
			help(args[1]);
			break;
		}
		case 3: {
			printHistory();
		}
		case 4:
		case 5: {
			source(args[1]);
			break;
		}
		default:
			return 0;
	}
	return 1;
}

int commandExec(int argc, char **args, int* p_in, int* p_out) {

	if (builtInCmdExec(argc, args))
		return -1;

	int pid = fork();
	if (pid < 0) {			//Error
		printf("Couldn't fork\n");
		return -1;
	}
	else if (pid > 0) {		//Parent process
		if (p_in[0] != -1)
			DBG_checkClose(close(p_in[0]), " input", "r", "parent", args[0]);
		if (p_in[1] != -1)
			DBG_checkClose(close(p_in[1]), " input", "w", "parent", args[0]);
		return pid;
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
			if (DEBUG_LEVEL > -1)
				printf("Couldn't execute %s\n", args[0]);
			exit(1);
		}
	}
}

int tokenise(char* cmd, char* delim, char **args, int tokeniseQuotes) {
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

			if (tokeniseQuotes) {
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

void pipesParser(char *c, int isBackground) {
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
		if (isBackground)
			signal(SIGINT, SIG_IGN);
		else
			signal(SIGINT, SIG_DFL);

		int piped_pid = commandExec(argc, args, pipes[i], pipes[i+1]);

		signal(SIGINT, reset);

		addProcess(piped_pid, isBackground);
	}
	if (!isBackground)
		for (process *p = head; p; p = p->next)
			waitpid(p->pid, 0, 0);
}

void statementsParser(char *c) {
	int last = strlen(c) - 1;
	int isBackground = 0;
	if (c[last] == '&' && c[last - 1] != '&') {
		c[last] = '\0';
		if (isspace(c[last - 1]))
			c[last - 1] = '\0';
		isBackground = 1;
	}

	char str[CMD_MAX_LEN], *statements[MAX_STATEMENTS];
	strcpy(str, c);

	int num_statements = tokenise(str, ";", statements, 0);
	DBG_checkStatements(num_statements, statements);

	for (int i = 0; i < num_statements; ++i) {
		if (i == num_statements - 1)
			pipesParser(statements[i], isBackground);
		else
			pipesParser(statements[i], 0);
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
	}
	strcpy(last_cmd, command);
}

char* preprocessCmd(char *cmd) {
	char *delims[] = {">>", ">", "<"/*, "&&", "||"*/};
	int n = sizeof(delims) / sizeof(delims[0]), m = strlen(cmd);
	int insertedLast = 0;

	char *new_cmd = malloc(2 * strlen(cmd) + CMD_MAX_LEN);
	DBG_checkMalloc(new_cmd);

	int i = 0, j = 0;

	while (isspace(cmd[i])) ++i;	//Trim leading spaces

	while (i < m) {

		if (i < m && strchr("\"\'", cmd[i])) {	//Skipping quotes
			char c = cmd[i];
			do {
				new_cmd[j++] = cmd[i++];
			} while (cmd[i] != c && i < m);
		}

		if (strncmp(cmd + i, "!!", 2) == 0 && i < m) {	//Replacing !! with last command
			insertedLast = 1;
			updateLast();
			new_cmd[j] = '\0';
			strcat(new_cmd, last_cmd);
			i += 2;
			j += strlen(last_cmd);
		}

		for (int k = 0; k < n; ++k) {	//Inserting spaces between operators
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
					new_cmd[j] = ' ';

				break;
			}
		}
		while (isspace(cmd[i]) && isspace(cmd[i + 1])) ++i;	//Truncate multiple spaces

		new_cmd[j++] = cmd[i++];
	}

	while (isspace(new_cmd[j - 1])) --j;	//Trim trailing spaces

	new_cmd[j] = '\0';

	if (insertedLast)
		printf("%s\n", new_cmd);

	return new_cmd;
}

char* readCommand(){
	char *prompt = promptString(), *c;
	if (use_readline) {
		c = readline(prompt);
		free(prompt);

		if (c == NULL)
			closeShell(0);

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
			closeShell(0);

		size_t len = strlen(c);
		if (c[len - 1] == '\n')
			c[len - 1] = '\0';
		else {
			while ((getchar()) != '\n');
			printf("Argument list too long, input dicarded\n");
			c[0] = '\0';
		}
	}
	char *cmd = preprocessCmd(c);

	if (strlen(cmd) || SAVE_EMPTY_CMD)
		addToHistory(cmd);

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
		while (sigsetjmp(ctrlc_buf, 1) != 0);

		signal(SIGCHLD, SIG_IGN);
		signal(SIGINT, reset);

		char* cmd = readCommand();

		if (strlen(cmd) == 0) {
			free(cmd);
			continue;
		}

		printf(RESET);
		fflush(stdout);

		statementsParser(cmd);
		free(cmd);
	}
}

int main(int argc, char const *argv[]) {
	if (argc > 1)
		use_readline = atoi(argv[1]);
	myShell();
	return 0;
}

// main -> myShell --(init)-> readCommand --(promptString, preprocessCmd, addToHistory) -> statementsParser -> 
// pipesParser --(initPipes, tokenise)-> commandExec -> builtInCmdExec 
// --(closeShell, execLast, help, printHistory, source)-> handleRedirect


		// printf("Your command: \'%s\' of length %ld\n", cmd, strlen(cmd));