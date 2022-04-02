/*
Compile using:
gcc -pthread prdcr_cnsmr.c -o prdcr_cnsmr.o
Run using:
./prdcr_cnsmr.o
*/

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <time.h>

#define BUFFER_SIZE 5
#define PRODUCE_TIME 0.1 * 1e6
#define CONSUME_TIME 0.1 * 1e6
#define TOTAL_ITEMS 10

sem_t empty_slots;
sem_t full_slots;
pthread_mutex_t buffer_mutex;

int *buffer;
int in = 0, out = 0;

void *producer(void *arg) {
	int total_items = *((int *)arg);
	int items_produced = 0;
	int x = 0;

	while (items_produced < total_items) {
		int next_produced = x++;
		usleep(PRODUCE_TIME);

		sem_wait(&empty_slots);
		pthread_mutex_lock(&buffer_mutex);

		buffer[in] = next_produced;
		in = (in + 1) % BUFFER_SIZE;
		printf("[+] Inserted %d\n", next_produced);
		items_produced++;

		pthread_mutex_unlock(&buffer_mutex);
		sem_post(&full_slots);

	}
}

void *consumer(void *arg) {
	int total_items = *((int *)arg);
	int items_consumed = 0;

	while (items_consumed < total_items) {
		sem_wait(&full_slots);
		pthread_mutex_lock(&buffer_mutex);

		int next_consumed = buffer[out];
		out = (out + 1) % BUFFER_SIZE;
		printf("[-] Taken 	%d\n", next_consumed);
		items_consumed++;

		pthread_mutex_unlock(&buffer_mutex);
		sem_post(&empty_slots);

		usleep(CONSUME_TIME);
	}
}

int main(int argc, char const *argv[]) {
	int total_items = TOTAL_ITEMS;
	if (argc == 2) {
		int temp = atoi(argv[1]);
		if (temp > 0)
			total_items = temp;
	}

	buffer = (int *)calloc(BUFFER_SIZE, sizeof(int));

	sem_init(&empty_slots, 0, BUFFER_SIZE);
	sem_init(&full_slots, 0, 0);
	pthread_mutex_init(&buffer_mutex, NULL);

	pthread_t prdcr_id, cnsmr_id;
	pthread_create(&prdcr_id, NULL, producer, &total_items);
	pthread_create(&cnsmr_id, NULL, consumer, &total_items);

	pthread_join(prdcr_id, NULL);
	pthread_join(cnsmr_id, NULL);

	sem_destroy(&empty_slots);
	sem_destroy(&full_slots);
	pthread_mutex_destroy(&buffer_mutex);
	return 0;
}