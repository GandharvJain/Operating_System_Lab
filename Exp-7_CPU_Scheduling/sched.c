/*
Compile using:
gcc sched.c -o sched.o
Run using:
./sched.o
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

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
	int curr_time = tasks[0].proc->arv_time;
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
	double avg_wt_time = (double)total_wt_time / n;
	double avg_ta_time = (double)total_ta_time / n;
	printf("Average turnaround time: %lf\n", avg_ta_time);
	printf("Average wait time: %lf\n\n", avg_wt_time);

	free(run_time);
}

int compareArrival(const void *e1, const void *e2) {
	if (sizeof(*e1) == t_struct_size) {
		Task *t1 = (Task *)e1;
		Task *t2 = (Task *)e2;
		return t1->proc->arv_time - t2->proc->arv_time;
	}
	else {
		Process *p1 = (Process *)e1;
		Process *p2 = (Process *)e2;
		return p1->arv_time - p2->arv_time;
	}
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

void schedSRTF(Process processes[], const int n) {
	Process *p = malloc(p_struct_size * (n + 1));
	memcpy(p, processes, p_struct_size * n);
	strcpy(p[n].name, "Error!");
	p[n].brst_time = INT_MAX;
	p[n].arv_time = INT_MAX;

	int num_tasks = 0, max_tasks = n;
	Task *tasks = malloc(t_struct_size * max_tasks);

	qsort(p, n, p_struct_size, compareArrival);

	int time_left[n+1];
	for (int i = 0; i <= n; ++i)
		time_left[i] = p[i].brst_time;

	int terminated = 0, shortest = 0, is_idle = 0, curr_task_run_time = 0, previous = 0;
	for (int curr_time = 0; terminated < n; ++curr_time) {
		int context_switched = 0;
		int min_time = time_left[shortest];

		for (int i = 0; i < n; ++i) {
			if (curr_time < p[i].arv_time)
				break;
			if (time_left[i] < min_time && time_left[i] != 0) {
				shortest = i;
				is_idle = 0;
			}
		}
		if (previous == n)
			previous = shortest;
		else if (previous != shortest) {
			context_switched = 1;
			time_left[shortest]++;
			curr_task_run_time--;
		}
		if (is_idle)
			continue;

		time_left[shortest]--;
		curr_task_run_time++;

		if (time_left[shortest] == 0) {
			terminated++;
			context_switched = 1;
			is_idle = 1;
			p[shortest].trm_time = curr_time + 1;
			shortest = n;
		}

		if (context_switched) {
			if (num_tasks == max_tasks) {
				max_tasks *= 2;
				tasks = realloc(tasks, t_struct_size * max_tasks);
			}
			tasks[num_tasks].p_index = previous;
			tasks[num_tasks].task_time = curr_task_run_time;
			tasks[num_tasks].proc = &p[previous];
			num_tasks++;
			curr_task_run_time = 0;
			previous = shortest;
		}
	}

	printf("\nSRTF:\n");
	schedInfo(p, n, tasks, num_tasks);
	free(p);
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
	for (int i = 0; i < n; ++i)
		scanf("%s %d %d", p[i].name, &p[i].brst_time, &p[i].arv_time);

	schedFCFS(p, n);
	schedSJF(p, n);
	schedSRTF(p, n);

	free(p);
	return 0;
}