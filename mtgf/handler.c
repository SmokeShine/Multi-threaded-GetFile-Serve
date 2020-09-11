#include "gfserver-student.h"
#include "gfserver.h"
#include "content.h"

#include "workload.h"

//
//  The purpose of this function is to handle a get request
//
//  The ctx is a pointer to the "context" operation and it contains connection state
//  The path is the path being retrieved
//  The arg allows the registration of context that is passed into this routine.
//  Note: you don't need to use arg. The test code uses it in some cases, but
//        not in others.
//

// steque_t* list;
// int run_thread_a;
// pthread_mutex_t run_lock_a;
// pthread_cond_t run_cond_a;

// int run_thread_b;
// pthread_mutex_t run_lock_b;
// pthread_cond_t run_cond_b;

steque_t *list;

int run_thread_a;
pthread_mutex_t run_lock_a;
pthread_cond_t run_cond_a;

int run_thread_b;
pthread_mutex_t run_lock_b;
pthread_cond_t run_cond_b;





gfh_error_t gfs_handler(gfcontext_t **ctx, const char *path, void *arg)
{

// // caller
// int a = 10;
// int *p = &a;
// callee_function(&p);
// // callee
// callee_function(int **p){
//   *p = NULL;
// }
	// enqueue request with struct
	steque_package *package;
	package = (steque_package *)malloc(sizeof(steque_package));
	package->ctx = *ctx;
	package->path = path;
	package->arg = arg;

	// (*(*ctx)).;
	// printf("Path Received is %s \n",path);
	// printf("CTX sizeof is %ld\n",sizeof(*ctx));
	
	pthread_mutex_lock(&run_lock_b);
	// Lock Acquired to update queue
	// Let's queue more to the workers	
	steque_enqueue(list, package);
	// printf("Current length of list is %d\n",steque_size(list));
	/* Now wake thread B */
	// run_thread_b = steque_size(list);
	pthread_cond_signal(&run_cond_b); //broadcast
	pthread_mutex_unlock(&run_lock_b);
	*ctx=NULL;
	// free(ctx); broken
	return gfh_success;
}
