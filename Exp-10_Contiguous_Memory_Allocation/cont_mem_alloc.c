/*
Programming project of Chapter 9 in Operating Systems Concepts Silberschatz (10th ed.)
Compile using:
gcc cont_mem_alloc.c -o cont_mem_alloc.o
Run using:
./cont_mem_alloc.o [NUMBER OF BYTES]
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

typedef int bool;
#define true 1
#define false 0

#define MAX_NAME_LEN 1024

typedef struct Mem_Block {
	char name[MAX_NAME_LEN];
	int start, size;
	struct Mem_Block *next, *prev;
} Mem_Block;

Mem_Block *head = NULL;

Mem_Block* createMemoryBlock(char* name, int start, int rng, Mem_Block* nxt, Mem_Block* prv);
void userMenu(void);
bool requestMemory(char* process_name, int amount, char fit);
bool releaseMemory(char* process_name);
void printStatus(void);
void compactMemory(void);
Mem_Block* findHole(int amount, char fit);

int main(int argc, char const *argv[]) {
	if (argc < 2) {
		printf("Usage: %s [NUMBER OF BYTES]\n", argv[0]);
		return 1;
	}
	int n = atoi(argv[1]);
	if (n < 1) {
		printf("Enter a positive integer\n");
		return 1;
	}
	head = createMemoryBlock("Unused", 0, n, NULL, NULL);

	bool running = true;
	while (running) {
		userMenu();
		char *cmd;
		int n = scanf("%ms", &cmd);
		printf("\n");
		if (strcmp(cmd, "RQ") == 0) {
			int amount;
			char *process_name, fit;
			scanf("%ms", &process_name);
			scanf("%d", &amount);
			scanf(" %c", &fit);

			if(requestMemory(process_name, amount, fit) == true)
				printf("Request satisfied!\n");
			else
				printf("Request denied!\n");
			free(process_name);
		}
		else if (strcmp(cmd, "RL") == 0) {
			char *process_name;
			scanf("%ms", &process_name);

			if(releaseMemory(process_name))
				printf("Memory released!\n");
			else
				printf("Memory not released!\n");
			free(process_name);
		}
		else if (strcmp(cmd, "STAT") == 0) {
			printf("Memory snapshot:-\n");
			printStatus();
		}
		else if (strcmp(cmd, "C") == 0) {
			compactMemory();
			printf("Memory compacted!\n");
		}
		else if (strcmp(cmd, "X") == 0) {
			printf("Exiting..\n");
			running = false;
		}
		else printf("Invalid command!\n");

		if (n == 1) free(cmd);
		printf("\n");
		while ((getchar()) != '\n');
	}

	return 0;
}

void userMenu() {
	printf("Request Memory : ");
	printf("\"RQ [PROCESS NAME] [NUMBER OF BYTES] [FIT ('F', 'B' or 'W')]\"\n");
	printf("Release Memory : \"RL [PROCESS NAME]\"\n");
	printf("Compact Memory : \"C\"\n");
	printf("Memory status  : \"STAT\"\n");
	printf("Exit           : \"X\"\n");
	printf("allocator> ");
}

Mem_Block* findHole(int amount, char fit) {
	Mem_Block *hole = NULL, *curr = head;
	if (fit == 'F') {
		for (; curr != NULL; curr = curr->next) {
			if (strcmp(curr->name, "Unused") != 0)
				continue;
			if (curr->size >= amount) {
				hole = curr;
				break;
			}
		}
	}
	else if (fit == 'B') {
		for (int min_size = INT_MAX; curr != NULL; curr = curr->next) {
			if (strcmp(curr->name, "Unused") != 0)
				continue;
			if (curr->size < min_size && curr->size >= amount) {
				min_size = curr->size;
				hole = curr;
			}
		}
	}
	else if (fit == 'W') {
		for (int max_size = amount - 1; curr != NULL; curr = curr->next) {
			if (strcmp(curr->name, "Unused") != 0)
				continue;
			if (curr->size > max_size) {
				max_size = curr->size;
				hole = curr;
			}
		}
	}
	return hole;
}

bool requestMemory(char* process_name, int amount, char fit) {
	if (amount < 1) {
		printf("Size should be positive\n");
		return false;
	}
	if (strchr("FBW", fit) == NULL) {
		printf("Invalid fit startegy\n");
		return false;
	}
	Mem_Block *hole = findHole(amount, fit), *new_block;
	if (hole == NULL) {
		printf("Insufficient memory!\n");
		return false;
	}
	if (hole->size == amount) {
		strcpy(hole->name, process_name);
		return true;
	}
	new_block = createMemoryBlock(process_name, hole->start, amount, hole, hole->prev);
	hole->start += amount;
	hole->size -= amount;
	hole->prev = new_block;
	if (new_block->prev != NULL)
		new_block->prev->next = new_block;
	if (head == hole)
		head = new_block;
	return true;
}

bool releaseMemory(char* process_name) {
	Mem_Block* curr = head;
	for (; curr != NULL; curr = curr->next)
		if (strcmp(curr->name, process_name) == 0)
			break;
	if (curr == NULL) {
		printf("No process with name \"%s\"\n", process_name);
		return false;
	}
	strcpy(curr->name, "Unused");
	Mem_Block *prv = curr->prev, *nxt = curr->next;
	if (nxt != NULL && strcmp(nxt->name, "Unused") == 0) {
		curr->next = nxt->next;
		if (nxt->next != NULL)
			nxt->next->prev = curr;
		curr->size += nxt->size;
		free(nxt);
	}
	if (prv != NULL && strcmp(prv->name, "Unused") == 0) {
		prv->next = curr->next;
		if (curr->next != NULL)
			curr->next->prev = prv;
		prv->size += curr->size;
		free(curr);
	}
	return true;
}

void compactMemory() {
	int curr_address = 0, unused_size = 0;
	Mem_Block *curr = head, *prv = NULL, *nxt = NULL;
	while (curr != NULL) {
		prv = curr->prev;
		nxt = curr->next;
		if (strcmp(curr->name, "Unused") == 0) {
			unused_size += curr->size;

			if (prv != NULL) prv->next = nxt;
			if (nxt != NULL) nxt->prev = prv;
			if (curr == head) head = nxt;
			free(curr);
		}
		else {
			curr->start = curr_address;
			curr_address += curr->size;
		}
		curr = nxt;
	}
	if (prv->next != NULL)
		prv = prv->next;
	Mem_Block *temp = createMemoryBlock("Unused", curr_address, unused_size, NULL, prv);
	if (prv == NULL)
		head = temp;
	else
		prv->next = temp;
}

void printStatus() {
	for (Mem_Block* curr = head; curr != NULL; curr = curr->next) {
		int start = curr->start;
		int end = start + curr->size - 1;
		char *str = "Process: ";
		if (strcmp(curr->name, "Unused") == 0) str = "";

		printf("Addresses [%d:%d] %s%s\n", start, end, str, curr->name);
	}
}

Mem_Block* createMemoryBlock(char* name, int start, int rng, Mem_Block* nxt, Mem_Block* prv) {
	Mem_Block* new_block = malloc(sizeof(Mem_Block));
	strcpy(new_block->name, name);
	new_block->start = start;
	new_block->size = rng;
	new_block->next = nxt;
	new_block->prev = prv;
	return new_block;
}