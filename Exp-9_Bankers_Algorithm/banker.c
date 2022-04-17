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

#define NUM_THREADS 5
#define NUM_RESOURCES 3

typedef int bool;
#define true 1
#define false 0

int available[NUM_RESOURCES];
int max[NUM_THREADS][NUM_RESOURCES];
int allocation[NUM_THREADS][NUM_RESOURCES];
int need[NUM_THREADS][NUM_RESOURCES];
int n_thrds = NUM_THREADS, n_rsrcs = NUM_RESOURCES;

bool requestResources(int thread_id, int request[]);
bool releaseResources(int thread_id, int release[]);

void addArray(int array_1[], int array_2[], int n, int multiplier);
bool isLessThanEqualTo(int array_1[], int array_2[], int n);
void printState(void);
void userMenu(void);
bool isSafeState(void);

int main(int argc, char const *argv[]) {
	if (argc < n_rsrcs + 1) {
		printf("Insufficient arguments!\n");
		printf("Usage: %s [INSTANCES_OF_R1] ... [INSTANCES_OF_R%d]\n", argv[0], n_rsrcs);
		return 1;
	}
	for (int i = 0; i < n_rsrcs; ++i) {
		available[i] = atoi(argv[i + 1]);
		if (available[i] < 0) {
			printf("Enter a non-negative integer for available\n");
			return 1;
		}
	}

	FILE* ptr = fopen("in.txt", "r");
	for (int i = 0; i < n_thrds; ++i) {
		for (int j = 0; j < n_rsrcs; ++j) {
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
	printState();
	printf("\n");

	while (running) {
		userMenu();
		char *cmd;
		int n = scanf("%ms", &cmd);
		if (strcmp(cmd, "RQ") == 0) {
			int thread_id, request[n_rsrcs];
			scanf("%d", &thread_id);
			thread_id--;
			for (int i = 0; i < n_rsrcs; ++i)
				scanf("%d", &request[i]);

			if(requestResources(thread_id, request) == true)
				printf("Request satisfied!\n");
			else
				printf("Request denied!\n");
		}
		else if (strcmp(cmd, "RL") == 0) {
			int thread_id, release[n_rsrcs];
			scanf("%d", &thread_id);
			thread_id--;
			for (int i = 0; i < n_rsrcs; ++i)
				scanf("%d", &release[i]);

			if(releaseResources(thread_id, release))
				printf("Resources released!\n");
			else
				printf("Resources not released!\n");
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
		printf("\n");
	}
	return 0;
}

void userMenu() {
	printf("Request resources : ");
	printf("\"RQ [CUSTOMER] [INSTANCES_OF_R1] ... [INSTANCES_OF_R%d]\"\n", n_rsrcs);
	printf("Release resources : ");
	printf("\"RL [CUSTOMER] [INSTANCES_OF_R1] ... [INSTANCES_OF_R%d]\"\n", n_rsrcs);
	printf("Print state       : \"*\"\n");
	printf("Exit              : \"X\"\n");
	printf("Enter command:\n");
}

void printState() {
	int padding = n_rsrcs * 2 + 1;
	printf("Thread-ID%*s %*s %*s\n", padding, "Used ", padding, "Max ", padding, "Need ");
	for (int i = 0; i < n_thrds; ++i) {
		printf("   %-5d:", i + 1);
		for (int j = 0; j < n_rsrcs; ++j)
			printf(" %d", allocation[i][j]);
		printf(" |");
		for (int j = 0; j < n_rsrcs; ++j)
			printf(" %d", max[i][j]);
		printf(" |");
		for (int j = 0; j < n_rsrcs; ++j)
			printf(" %d", need[i][j]);
		printf("\n");
	}
	printf("Available: ");
	for (int i = 0; i < n_rsrcs; ++i)
		printf(" %d", available[i]);
	printf("\n");
}

bool isLessThanEqualTo(int array_1[], int array_2[], int n) {
	bool is_less_or_equal = true;
	for (int i = 0; i < n; ++i) {
		if (array_1[i] > array_2[i]) {
			is_less_or_equal = false;
			break;
		}
	}
	return is_less_or_equal;
}

void addArray(int array_1[], int array_2[], int n, int multiplier) {
	for (int i = 0; i < n; ++i)
		array_1[i] += multiplier * array_2[i];
}

bool isSafeState() {
	int work[n_rsrcs];
	bool finish[n_thrds];
	memset(work, 0, sizeof(work));
	memset(finish, false, sizeof(finish));
	addArray(work, available, n_rsrcs, +1);

	while (true) {
		int ind = -1;
		for (int i = 0; i < n_thrds; ++i) {
			if (!finish[i] && isLessThanEqualTo(need[i], work, n_rsrcs)) {
				ind = i;
				break;
			}
		}
		if (ind == -1) break;

		addArray(work, allocation[ind], n_rsrcs, +1);
		finish[ind] = true;
	}

	for (int i = 0; i < n_thrds; ++i)
		if (!finish[i])
			return false;
	return true;
}

bool requestResources(int thread_id, int request[]) {
	if (!isLessThanEqualTo(request, need[thread_id], n_rsrcs)) {
		printf("Exceeded claim!\n");
		return false;
	}
	if (!isLessThanEqualTo(request, available, n_rsrcs)) {
		printf("Not enough resources!\n");
		return false;
	}

	addArray(available, request, n_rsrcs, -1);
	addArray(allocation[thread_id], request, n_rsrcs, +1);
	addArray(need[thread_id], request, n_rsrcs, -1);

	if (!isSafeState) {
		releaseResources(thread_id, request);
		printf("Unsafe state!\n");
		return false;
	}
	return true;
}

bool releaseResources(int thread_id, int release[]) {
	if (!isLessThanEqualTo(release, allocation[thread_id], n_rsrcs)) {
		printf("Cannot release more than allocated!\n");
		return false;
	}
	addArray(available, release, n_rsrcs, +1);
	addArray(allocation[thread_id], release, n_rsrcs, -1);
	addArray(need[thread_id], release, n_rsrcs, +1);
	return true;
}