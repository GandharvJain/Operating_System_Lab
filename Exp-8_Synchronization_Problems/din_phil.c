/*
Compile using:
gcc -pthread din_phil.c -o din_phil.o
Run using:
./din_phil.o [PHILOSOPHERS]
*/

#include <stdio.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>

#define EAT_DURATION (3 + rand() % 4)
#define THINK_DURATION (6 + rand() % 5)

void eat(int);
void think(int);

int n;
sem_t attempting_to_eat;
sem_t *chopstick;

sem_t print_sem;
int *eating;
void printState(int, int);

void *philosopher(void *arg) {
	int phil_id = *((int *)arg);
	int left = phil_id;
	int right = (phil_id + 1) % n;

	while (1) {
		sem_wait(&attempting_to_eat);

		sem_wait(&chopstick[left]);
		sem_wait(&chopstick[right]);

		printState(phil_id, 1);
		eat(phil_id);
		printState(phil_id, 0);

		sem_post(&chopstick[left]);
		sem_post(&chopstick[right]);

		sem_post(&attempting_to_eat);

		think(phil_id);
	}
}

void *philosopher_asymmetric(void *arg) {
	int phil_id = *((int *)arg);
	int is_odd = phil_id % 2;
	int left = phil_id;
	int right = (phil_id + 1) % n;

	while (1) {
		if (is_odd) {
			sem_wait(&chopstick[left]);
			sem_wait(&chopstick[right]);
		}
		else {
			sem_wait(&chopstick[right]);
			sem_wait(&chopstick[left]);
		}

		printState(phil_id, 1);
		eat(phil_id);
		printState(phil_id, 0);

		sem_post(&chopstick[left]);
		sem_post(&chopstick[right]);

		think(phil_id);
	}
}

int main(int argc, char const *argv[]) {
	if (argc < 2) {
		printf("Usage: %s [PHILOSOPHERS]\n", argv[0]);
		return 1;
	}
	n = atoi(argv[1]);
	if (n < 1) {
		printf("Enter a positive integer\n");
		return 1;
	}

	srand(time(NULL));
	chopstick = (sem_t *)calloc(n, sizeof(sem_t));

	sem_init(&print_sem, 0, 1);
	eating = (int *)calloc(n, sizeof(int));

	sem_init(&attempting_to_eat, 0, n - 1);
	for (int i = 0; i < n; ++i)
		sem_init(&chopstick[i], 0, 1);

	int phil_id[n];
	pthread_t thread_ids[n];

	for (int i = 0; i < n; ++i) {
		phil_id[i] = i;
		pthread_create(&thread_ids[i], NULL, philosopher, &phil_id[i]);
	}

	for (int i = 0; i < n; ++i)
		pthread_join(thread_ids[i], NULL);

	sem_destroy(&print_sem);
	sem_destroy(&attempting_to_eat);
	for (int i = 0; i < n; ++i)
		sem_destroy(&chopstick[i]);

	return 0;
}

void eat(int phil_id) {
	sleep(EAT_DURATION);
}

void think(int phil_id) {
	sleep(THINK_DURATION);
}

void printState(int phil_id, int val) {
	sem_wait(&print_sem);
	// if (val)
	// 	printf("Philosopher %d is eating\n", phil_id);
	// else
	// 	printf("Philosopher %d finished eating\n", phil_id);

	eating[phil_id] = val;
	for (int i = 0; i < n; ++i)
		printf("%d ", eating[i]);
	printf("\n");
	sem_post(&print_sem);
}