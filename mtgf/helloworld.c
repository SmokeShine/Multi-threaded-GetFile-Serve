#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <dirent.h>
#include <time.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <pthread.h>
#include <assert.h>
/*Two threads. One for game, another for timer*/
#define NUM_THREADS 2

FILE *fptr;

/*Mutex Mechanism for controlling timer functionality*/
int run_thread_a = 0;
pthread_mutex_t run_lock_a = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t run_cond_a = PTHREAD_COND_INITIALIZER;

int run_thread_b = 0;
pthread_mutex_t run_lock_b = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t run_cond_b = PTHREAD_COND_INITIALIZER;

void *mainGame(void *arguments)
{
	/*No Restarts - Single Finish Game*/
	while (1)
	{
		pthread_mutex_lock(&run_lock_b);

		while (!run_thread_b)
			pthread_cond_wait(&run_cond_b, &run_lock_b);
		printf("Thread ID is: %ld working on %d\n", (long)pthread_self(), run_thread_b);
		pthread_mutex_unlock(&run_lock_b);
		sleep(1);
	}
}

int main()
{
	/* Create two threads*/

	pthread_t threads_a;
	pthread_t threads_b;
	/*Initialize threads*/
	pthread_create(&threads_a, NULL, mainGame, NULL);
	pthread_create(&threads_b, NULL, mainGame, NULL);
	/*Wait for game to finish*/
	for (int i = 0; i < 10; i++)
	{
		pthread_mutex_lock(&run_lock_b);
		run_thread_b = i % 2;
		pthread_cond_signal(&run_cond_b);
		pthread_mutex_unlock(&run_lock_b);
	}
	pthread_join(threads_a, NULL);
	/*Exit main*/
	return 0;
}
