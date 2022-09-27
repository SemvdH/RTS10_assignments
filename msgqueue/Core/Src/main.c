#include "main.h"
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <mqueue.h>
#include <fcntl.h>

#define SIZE 8

struct character_mq {
	mqd_t *mgdesPtr;
	char c;
};

void check_errno(int error) {
	if (error < 0) {
		perror("Error");
		while (1)
			;
	}
}

void check(int error) {
	if (error != 0) {
		printf("Error: %s\n", strerror(error));
		while (1)
			;
	}
}

//char buffer[SIZE]; // buffer which can store SIZE elements of type char
//int indexGet = 0; // index where the next element will be read
//int indexPut = 0; // index where the next element will be written
//sem_t semMutualExclusive; // binary semaphore: used for mutual exclusive use of the buffer
//sem_t semEmpty; // counting semaphore: counts the number of empty places
//sem_t semFilled; // counting semaphore: count the number of filled places
//sem_t semPrintf; // binary semaphore: used for mutual exclusive use of printf
pthread_mutex_t m;

void put(char c, mqd_t *mqd) {
//    check_errno( sem_wait(&semEmpty) ); // lower the number of empty places, WAIT if there are no free places left!
//    check_errno( sem_wait(&semMutualExclusive) ); // enter critical region
//    buffer[indexPut] = c;
	check_errno(mq_send(*mqd, &c, sizeof(c), 0));
//	indexPut++;
//	if (indexPut == SIZE) {
//		indexPut = 0;
//	}
//    check_errno( sem_post(&semMutualExclusive) ); // leave critical region
//    check_errno( sem_post(&semFilled) ); // increase the number of filled places
}

char get(mqd_t *mq) {
//    check_errno( sem_wait(&semFilled) ); // lower the number of filled places, WAIT if there are no filled places left!
//    check_errno( sem_wait(&semMutualExclusive) ); // enter critical region
//    char c = buffer[indexGet];
	char msg;
	check_errno(mq_receive(*mq, (char*) &msg, sizeof(msg), NULL));
//	indexGet++;
//	if (indexGet == SIZE) {
//		indexGet = 0;
//	}
//    check_errno( sem_post(&semMutualExclusive) ); // leave critical region
//    check_errno( sem_post(&semEmpty) ); // increase the number of empty places
	return msg;
}

void* producer(void *arg) // function for producer thread
{
	struct character_mq cmq = *(struct character_mq*) arg;
	pthread_mutex_lock(&m);
	check_errno(
			printf("Thread: %p with argument: %c starts\n", pthread_self(),
					cmq.c));
	pthread_mutex_unlock(&m);
	for (int i = 0; i < 100; ++i) {
		put(cmq.c, cmq.mgdesPtr);
	}
	pthread_mutex_lock(&m);
	check_errno(printf("Thread: %p stops\n", pthread_self()));
	pthread_mutex_unlock(&m);
	return NULL;
}

void* consumer(void *arg) // function for consumer thread
{
	mqd_t mq = *(mqd_t*) arg;
	pthread_mutex_lock(&m);
	check_errno(printf("Thread: %p starts\n", pthread_self()));
	pthread_mutex_unlock(&m);
	for (int i = 0; i < 200; ++i) {
		char c = get(&mq);
		pthread_mutex_lock(&m);
		check_errno(printf("%c", c));
		check_errno(fflush(stdout));
		pthread_mutex_unlock(&m);
	}
	pthread_mutex_lock(&m);
	check_errno(printf("Thread: %p stops\n", pthread_self()));
	pthread_mutex_unlock(&m);
	return NULL;
}

int read_prio(char *process_name) {
	int prio;
	do {
		check_errno(
				printf("Enter priority for process %s [1..15]: ",
						process_name));
		check_errno(fflush(stdout));
	} while (scanf("%d", &prio) != 1 || prio < 1 || prio > 15);
	return prio;
}

void* main_thread(void *arg) {
	mqd_t mqdes;
	struct mq_attr mqAttrs;
	mqAttrs.mq_maxmsg = 8;
	mqAttrs.mq_msgsize = sizeof(char);
	mqAttrs.mq_flags = 0;
	check_errno(
			(int) (mqdes = mq_open("/snacks", O_RDWR | O_CREAT, 0666, &mqAttrs)));

	pthread_mutexattr_t ma;
	check(pthread_mutexattr_init(&ma));
	check(pthread_mutex_init(&m, &ma));

	int prioc = read_prio("Consumer");
	int priop1 = read_prio("Frikandel Producer");
	int priop2 = read_prio("Kroket Producer");

	check_errno(printf("Output for Consumer priority = %d ", prioc));
	check_errno(printf("frikandel Producer priority = %d ", priop1));
	check_errno(printf("Kroket Producer priority = %d\n", priop2));

	pthread_attr_t ptac, ptap1, ptap2;
	check(pthread_attr_init(&ptac));
	check(pthread_attr_init(&ptap1));
	check(pthread_attr_init(&ptap2));

	check(pthread_attr_setstacksize(&ptac, 1024));
	check(pthread_attr_setstacksize(&ptap1, 1024));
	check(pthread_attr_setstacksize(&ptap2, 1024));

	struct sched_param spc, spp1, spp2;
	check(pthread_attr_getschedparam(&ptac, &spc));
	check(pthread_attr_getschedparam(&ptap1, &spp1));
	check(pthread_attr_getschedparam(&ptap2, &spp2));

	spc.sched_priority = prioc;
	spp1.sched_priority = priop1;
	spp2.sched_priority = priop2;

	check(pthread_attr_setschedparam(&ptac, &spc));
	check(pthread_attr_setschedparam(&ptap1, &spp1));
	check(pthread_attr_setschedparam(&ptap2, &spp2));

	pthread_t ptc, ptp1, ptp2;
	struct character_mq frikandelMq = { &mqdes, 'F' };
	struct character_mq kroketMq = { &mqdes, 'K' };
	check(pthread_create(&ptc, &ptac, consumer, &mqdes));
	check(pthread_create(&ptp1, &ptap1, producer, &frikandelMq));
	check(pthread_create(&ptp2, &ptap2, producer, &kroketMq));

	check(pthread_join(ptc, NULL));
	check(pthread_join(ptp1, NULL));
	check(pthread_join(ptp2, NULL));

	check(pthread_attr_destroy(&ptac));
	check(pthread_attr_destroy(&ptap1));
	check(pthread_attr_destroy(&ptap2));

	check(pthread_mutexattr_destroy(&ma));
	check(pthread_mutex_destroy(&m));

	return NULL;
}

int main(void) {
	Board_Init();

	pthread_attr_t pta;
	check(pthread_attr_init(&pta));
	check(pthread_attr_setdetachstate(&pta, PTHREAD_CREATE_DETACHED));
	check(pthread_attr_setstacksize(&pta, 1024));

	struct sched_param sp;
	check(pthread_attr_getschedparam(&pta, &sp));
	// The main thread must have the highest priority because this thread will start
	// the other threads and we want to study the interaction between those other threads
	sp.sched_priority = 15;
	check(pthread_attr_setschedparam(&pta, &sp));

	pthread_t pt;
	check(pthread_create(&pt, &pta, main_thread, NULL));

	printf("\n");
	vTaskStartScheduler();
	/* We should never get here as control is now taken by the scheduler */

	check(pthread_attr_destroy(&pta));

	return EXIT_SUCCESS;
}
