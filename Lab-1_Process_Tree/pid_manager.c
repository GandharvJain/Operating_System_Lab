// Programming practice 3.20 in Operating Systems Concepts Silberschatz
// Compile using:
// gcc pid_manager.c -o pid_manager.o
// Run using:
// ./pid_manager.o

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <limits.h>

#define MIN_PID 5
#define MAX_PID 15

int bytesInBlock;
int bitsInBlock;
int maxProcesses = MAX_PID - MIN_PID + 1;
int numBlocks;

unsigned int *bitmap;
int pidPointer = 0;

int isSet(int pid) {
	int blockNum = pid / bitsInBlock;
	int bitNum = pid % bitsInBlock;
	int b = bitmap[blockNum] & (1 << bitNum);
	return b > 0;
}

void setPID(int pid) {
	int blockNum = pid / bitsInBlock;
	int bitNum = pid % bitsInBlock;
	bitmap[blockNum] = bitmap[blockNum] | (1 << bitNum);
}

int allocate_map() {
	bytesInBlock = sizeof(unsigned int);
	bitsInBlock =  bytesInBlock * CHAR_BIT;
	bitmap = calloc(numBlocks, bytesInBlock);
	numBlocks = (maxProcesses + bitsInBlock - 1) / bitsInBlock;
	pidPointer = 0;
	if (bitmap)
		return 1;
	return -1;
}

int allocate_pid() {
	int pid = pidPointer;
	while (isSet(pid)) {
	    pid = (pid + 1) % maxProcesses;
	    if (pid % maxProcesses == pidPointer)
	    	return -1;
	}
	setPID(pid);
	pidPointer = pid;
	return MIN_PID + pid;
}

void release_pid(int pid) {
	pid -= MIN_PID;
	int blockNum = pid / bitsInBlock;
	int bitNum = pid % bitsInBlock;
	bitmap[blockNum] &= ~(1 << bitNum);
}

void printBitmap() {
	for (int i = 0; i < maxProcesses; ++i)
		printf("%d", isSet(i));
	printf("\n");
}

int main(int argc, char const *argv[]) {
	if (allocate_map() < 0) {
		printf("Error allocating map\n");
		return 1;
	}
	printf("bitsInBlock: %d, bytesInBlock: %d\n", bitsInBlock, bytesInBlock);
	while (1) {
		printBitmap();
		printf("Enter option (pid < 0: exit, pid = 0: allocate_pid, pid > 0: free pid)\n");
		int c;
		scanf("%d", &c);
		int d;
		while ((d = getchar()) != '\n' && d != EOF);
		if (c == 0) {
			printf("PID allocated: %d\n", allocate_pid());
		}
		else if (c > 0) {
			release_pid(c);
		}
		else {
			return 0;
		}
	}
	return 0;
}