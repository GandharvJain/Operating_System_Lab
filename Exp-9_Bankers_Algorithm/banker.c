/*
Programming project of Chapter 8 in Operating Systems Concepts Silberschatz (10th ed.)
Compile using:
gcc banker.c -o banker.o
Run using:
./banker.o [INSTANCES_OF_R1] [INSTANCES_OF_R2] ...
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define NUM_CUSTOMERS 5
#define NUM_RESOURCES 4

typedef int bool;
#define true 1
#define false 0

int available[NUM_RESOURCES];
int max[NUM_CUSTOMERS][NUM_RESOURCES];
int allocation[NUM_CUSTOMERS][NUM_RESOURCES];
int need[NUM_CUSTOMERS][NUM_RESOURCES];

bool requestResources(int customer_num, int request[]);
void releaseResources(int customer_num, int release[]);
void printState(void);
void userMenu(void);

int main(int argc, char const *argv[]) {
	if (argc < NUM_RESOURCES + 1) {
		printf("Insufficient arguments!\n");
		printf("Usage: %s [INSTANCES_OF_R1] ... [INSTANCES_OF_R%d]\n", argv[0], NUM_RESOURCES);
		return 1;
	}
	for (int i = 0; i < NUM_RESOURCES; ++i) {
		available[i] = atoi(argv[i + 1]);
		if (available[i] < 0) {
			printf("Enter a non-negative integer for available\n");
			return 1;
		}
	}

	FILE* ptr = fopen("in.txt", "r");
	for (int i = 0; i < NUM_CUSTOMERS; ++i) {
		for (int j = 0; j < NUM_RESOURCES; ++j) {
			allocation[i][j] = 0;
			fscanf(ptr, "%d", &max[i][j]);
			need[i][j] = max[i][j];
			if (max[i][j] < 0) {
				printf("Enter a non-negative integer for maximum\n");
				return 1;
			}
		}
	}
	fclose(ptr);

	bool running = true;
	while (running) {
		userMenu();
		char *cmd;
		int n = scanf("%ms", &cmd);
		if (strcmp(cmd, "RQ") == 0) {
			int customer_num, request[NUM_RESOURCES];
			scanf("%d", &customer_num);
			for (int i = 0; i < NUM_RESOURCES; ++i)
				scanf("%d", &request[i]);

			if(requestResources(customer_num, request))
				printf("Request satisfied!\n");
			else
				printf("Request denied!\n");
		}
		else if (strcmp(cmd, "RL") == 0) {
			int customer_num, release[NUM_RESOURCES];
			scanf("%d", &customer_num);
			for (int i = 0; i < NUM_RESOURCES; ++i)
				scanf("%d", &release[i]);

			releaseResources(customer_num, release);
			printf("Resources released!\n");
		}
		else if (strcmp(cmd, "*") == 0) {
			printf("\nSystem state:-\n");
			printState();
		}
		else if (strcmp(cmd, "X") == 0) {
			printf("Exiting..\n");
			running = false;
		}
		else printf("Invalid command!\n");

		if (n == 1) free(cmd);
	}
	return 0;
}

void userMenu() {
	printf("Request resources : ");
	printf("\"RQ [CUSTOMER] [INSTANCES_OF_R1] ... [INSTANCES_OF_R%d]\"\n", NUM_RESOURCES);
	printf("Release resources : ");
	printf("\"RL [CUSTOMER] [INSTANCES_OF_R1] ... [INSTANCES_OF_R%d]\"\n", NUM_RESOURCES);
	printf("Print state       : \"*\"\n");
	printf("Exit              : \"X\"\n");
	printf("Enter command:\n");
}

void printState() {
	int padding = NUM_RESOURCES * 2 + 1;
	printf("Customer %*s %*s %*s\n", padding, "Used ", padding, "Max ", padding, "Need ");
	for (int i = 0; i < NUM_CUSTOMERS; ++i) {
		printf("   %-5d:", i + 1);
		for (int j = 0; j < NUM_RESOURCES; ++j)
			printf(" %d", allocation[i][j]);
		printf(" |");
		for (int j = 0; j < NUM_RESOURCES; ++j)
			printf(" %d", max[i][j]);
		printf(" |");
		for (int j = 0; j < NUM_RESOURCES; ++j)
			printf(" %d", need[i][j]);
		printf("\n");
	}
	printf("Available: ");
	for (int i = 0; i < NUM_RESOURCES; ++i)
		printf(" %d", available[i]);
	printf("\n\n");
}

bool requestResources(int customer_num, int request[]) {
	return false;
}

void releaseResources(int customer_num, int release[]) {

}