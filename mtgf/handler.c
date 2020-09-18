/*
 ============================================================================
 Name        : handler.c
 Author      : Prateek Gupta
 Version     : v1
 ============================================================================
 */

/*                                      File Usage - Ubuntu                                     */
/*
Compile: make clean all
Execute: -
Return: -
 The purpose of this function is to handle a get request
 The ctx is a pointer to the "context" operation and it contains connection state
 The path is the path being retrieved
 The arg allows the registration of context that is passed into this routine.
*/

#include "gfserver-student.h"
#include "gfserver.h"
#include "content.h"

#include "workload.h"

steque_t *list;

int run_thread_a;
pthread_mutex_t run_lock_a;
pthread_cond_t run_cond_a;

int run_thread_b;
pthread_mutex_t run_lock_b;
pthread_cond_t run_cond_b;

// Handler to send header and data to the client
gfh_error_t gfs_handler(gfcontext_t **ctx, const char *path, void *arg)
{
	steque_package *package;
	package = (steque_package *)malloc(sizeof(steque_package));
	package->ctx = *ctx;
	package->path = path;
	package->arg = arg;
	// Acquire mutex
	pthread_mutex_lock(&run_lock_b);
	// Update queue
	steque_enqueue(list, package);
	pthread_cond_signal(&run_cond_b);
	// Release mutex
	pthread_mutex_unlock(&run_lock_b);
	*ctx = NULL;
	return gfh_success;
}
