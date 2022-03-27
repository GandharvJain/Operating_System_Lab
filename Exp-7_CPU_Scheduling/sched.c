/*
Compile using:
gcc sched.c -o sched.o
Run using:
./sched.o
*/

#include <stdio.h>
#include <stdlib.h>
#define MAX_NAME_LEN 1024

typedef struct Process {
	char name[MAX_NAME_LEN];
	int brst_time, arv_time, trm_time;
} Process;
int p_struct_size = sizeof(Process);

typedef struct Task {
	int p_index, task_time;
	const Process *proc;
} Task;
int t_struct_size = sizeof(Task);


void schedInfo(Process p[], int n, const Task tasks[], int m) {
	int *run_time = calloc(n, sizeof(int));
	int curr_time = 0;
	printf("%7s %7s\n", "Process", "Duration");
	for (int i = 0; i < m; ++i) {
		int ind = tasks[i].p_index;

		curr_time += tasks[i].task_time;
		run_time[ind] += tasks[i].task_time;

		if (run_time[ind] == p[ind].brst_time)
			p[ind].trm_time = curr_time;

		printf("%7s %7d\n", p[ind].name, tasks[i].task_time);
	}

	int total_wt_time = 0;
	int total_ta_time = 0;
	for (int i = 0; i < n; ++i) {
		int ta_time = p[i].trm_time - p[i].arv_time;
		total_ta_time += ta_time;
		total_wt_time += ta_time - p[i].brst_time;
	}
	int avg_wt_time = total_wt_time / n;
	int avg_ta_time = total_ta_time / n;
	printf("Average turnaround time: %d\n", avg_ta_time);
	printf("Average wait time: %d\n\n", avg_wt_time);

	free(run_time);
}

int compareArrival(const void *e1, const void *e2) {
	Task *t1 = (Task *)e1;
	Task *t2 = (Task *)e2;
	return t1->proc->arv_time - t2->proc->arv_time;
}

void schedFCFS(Process p[], const int n) {
	Task *tasks = malloc(t_struct_size * n);
	for (int i = 0; i < n; ++i) {
		tasks[i].p_index = i;
		tasks[i].task_time = p[i].brst_time;
		tasks[i].proc = &p[i];
	}

	qsort(tasks, n, t_struct_size, compareArrival);

	printf("\nFCFS:\n");
	schedInfo(p, n, tasks, n);
	free(tasks);
}

int compareBurst(const void *e1, const void *e2) {
	Task *t1 = (Task *)e1;
	Task *t2 = (Task *)e2;
	return t1->proc->brst_time - t2->proc->brst_time;
}

void schedSJF(Process p[], const int n) {
	Task *tasks = malloc(t_struct_size * n);
	for (int i = 0; i < n; ++i) {
		tasks[i].p_index = i;
		tasks[i].task_time = p[i].brst_time;
		tasks[i].proc = &p[i];
	}

	qsort(tasks, n, t_struct_size, compareBurst);

	printf("\nSJF:\n");
	schedInfo(p, n, tasks, n);
	free(tasks);
}

int main(int argc, char const *argv[]) {
	int n;
	printf("Enter number of processes\n");
	scanf("%d", &n);
	int d;
	while ((d = getchar()) != '\n' && d != EOF);

	Process *p = malloc(p_struct_size * n);
	printf("Enter process name, burst time and arrival time (Space seperated): \n");
	for (int i = 0; i < n; ++i) {
		scanf("%s %d %d", p[i].name, &p[i].brst_time, &p[i].arv_time);
	}

	schedFCFS(p, n);
	schedSJF(p, n);

	return 0;
}