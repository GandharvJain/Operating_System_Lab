/*
Compile using:
gcc -pthread prdcr_cnsmr.c -o prdcr_cnsmr.o
Run using:
./prdcr_cnsmr.o [PRODUCERS] [CONSUMERS]
*/

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <time.h>

#define BUFFER_SIZE 5
#define PRODUCE_TIME 1
#define CONSUME_TIME 1
#define TOTAL_ITEMS 15

sem_t empty_slots;
sem_t full_slots;
pthread_mutex_t buffer_mutex;

int *buffer;
int in = 0, out = 0;

pthread_mutex_t producer_mutex;
sem_t produced_count, consumed_count;
int x = 0;

int produceItem(void);
void consumeItem(int);
int finishedProducing(void);
int finishedConsuming(void);

void *producer(void *arg) {
	while (1) {
		sem_post(&produced_count);
		if (finishedProducing()) break;

		int next_produced = produceItem();

		sem_wait(&empty_slots);
		pthread_mutex_lock(&buffer_mutex);

		buffer[in] = next_produced;
		in = (in + 1) % BUFFER_SIZE;
		printf("Inserted	%d\n", next_produced);

		pthread_mutex_unlock(&buffer_mutex);
		sem_post(&full_slots);
	}
}

void *consumer(void *arg) {
	while (1) {
		sem_post(&consumed_count);
		if (finishedConsuming()) break;

		sem_wait(&full_slots);
		pthread_mutex_lock(&buffer_mutex);

		int next_consumed = buffer[out];
		out = (out + 1) % BUFFER_SIZE;
		printf("Taken 	%d\n", next_consumed);

		pthread_mutex_unlock(&buffer_mutex);
		sem_post(&empty_slots);

		consumeItem(next_consumed);
	}
}

int main(int argc, char const *argv[]) {
	if (argc < 3) {
		printf("Usage: %s [PRODUCERS] [CONSUMERS]\n", argv[0]);
		return 1;
	}
	int num_producers = atoi(argv[1]), num_consumers = atoi(argv[2]);
	if (num_producers < 1 || num_consumers < 1) {
		printf("Enter a positive integer\n");
		return 1;
	}

	buffer = (int *)calloc(BUFFER_SIZE, sizeof(int));

	sem_init(&empty_slots, 0, BUFFER_SIZE);
	sem_init(&full_slots, 0, 0);
	sem_init(&produced_count, 0, 0);
	sem_init(&consumed_count, 0, 0);
	pthread_mutex_init(&buffer_mutex, NULL);
	pthread_mutex_init(&producer_mutex, NULL);

	pthread_t prdcr_id[num_producers], cnsmr_id[num_consumers];
	for (int i = 0; i < num_producers; ++i)
		pthread_create(&prdcr_id[i], NULL, producer, NULL);
	for (int i = 0; i < num_consumers; ++i)
		pthread_create(&cnsmr_id[i], NULL, consumer, NULL);

	for (int i = 0; i < num_producers; ++i)
		pthread_join(prdcr_id[i], NULL);
	for (int i = 0; i < num_consumers; ++i)
		pthread_join(cnsmr_id[i], NULL);

	sem_destroy(&empty_slots);
	sem_destroy(&full_slots);
	pthread_mutex_destroy(&buffer_mutex);
	return 0;
}

int produceItem() {
	pthread_mutex_lock(&producer_mutex);
	int item = x++;
	pthread_mutex_unlock(&producer_mutex);

	usleep(PRODUCE_TIME * 1e6);
	return item;
}
void consumeItem(int item) {
	usleep(CONSUME_TIME * 1e6);
}

int finishedProducing() {
	int cnt;
	sem_getvalue(&produced_count, &cnt);
	if (cnt > TOTAL_ITEMS)
		return 1;
	return 0;
}

int finishedConsuming() {
	int cnt;
	sem_getvalue(&consumed_count, &cnt);
	if (cnt > TOTAL_ITEMS)
		return 1;
	return 0;
}