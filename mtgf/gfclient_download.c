/*
 ============================================================================
 Name        : gfclient_download.c
 Author      : Prateek Gupta
 Version     : v1
 ============================================================================
 */

/*                                      File Usage - Ubuntu                                     */
/*
Compile: make clean all
Execute: ./gfclient_download
Return: -
Example:
root@8c9f19e07061:/workspace/pr1/mtgf# ./gfclient_download -h
usage:
  webclient [options]
options:
  -h                  Show this help message
  -r [num_requests]   Request download total (Default: 2)
  -p [server_port]    Server port (Default: 20121)
  -s [server_addr]    Server address (Default: 127.0.0.1)
  -t [nthreads]       Number of threads (Default 2)
  -w [workload_path]  Path to workload file (Default: workload.txt)
  */
#include "gfclient-student.h"

#define MAX_THREADS (2012)

#define USAGE                                                   \
  "usage:\n"                                                    \
  "  webclient [options]\n"                                     \
  "options:\n"                                                  \
  "  -h                  Show this help message\n"              \
  "  -r [num_requests]   Request download total (Default: 2)\n" \
  "  -p [server_port]    Server port (Default: 20121)\n"        \
  "  -s [server_addr]    Server address (Default: 127.0.0.1)\n" \
  "  -t [nthreads]       Number of threads (Default 2)\n"       \
  "  -w [workload_path]  Path to workload file (Default: workload.txt)\n"

/* OPTIONS DESCRIPTOR ====================================================== */
static struct option gLongOptions[] = {
    {"help", no_argument, NULL, 'h'},
    {"nthreads", required_argument, NULL, 't'},
    {"nrequests", required_argument, NULL, 'r'},
    {"server", required_argument, NULL, 's'},
    {"port", required_argument, NULL, 'p'},
    {"workload-path", required_argument, NULL, 'w'},
    {NULL, 0, NULL, 0}};

// Global Variables for thread control
pthread_mutex_t run_lock_a = PTHREAD_MUTEX_INITIALIZER;

pthread_mutex_t run_lock_b = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t run_cond_b = PTHREAD_COND_INITIALIZER;

steque_t *list;
sig_atomic_t volatile requests_left = -1;
char *server = "localhost";
unsigned short port = 20121;

static void Usage() { fprintf(stderr, "%s", USAGE); }

static void localPath(char *req_path, char *local_path)
{
  static int counter = 0;

  sprintf(local_path, "%s-%06d", &req_path[1], counter++);
}

static FILE *openFile(char *path)
{
  char *cur, *prev;
  FILE *ans;

  /* Make the directory if it isn't there */
  prev = path;
  while (NULL != (cur = strchr(prev + 1, '/')))
  {
    *cur = '\0';

    if (0 > mkdir(&path[0], S_IRWXU))
    {
      if (errno != EEXIST)
      {
        perror("Unable to create directory");
        exit(EXIT_FAILURE);
      }
    }

    *cur = '/';
    prev = cur;
  }

  if (NULL == (ans = fopen(&path[0], "w")))
  {
    perror("Unable to open file");
    exit(EXIT_FAILURE);
  }

  return ans;
}

/* Callbacks ========================================================= */
static void writecb(void *data, size_t data_len, void *arg)
{
  FILE *file = (FILE *)arg;

  fwrite(data, 1, data_len, file);
}

int check(){
    pthread_mutex_lock(&run_lock_a);
    int  x;
    x=requests_left ;
    pthread_mutex_unlock(&run_lock_a);
    return x;
}
// Worker Thread
void *worker(void *argument)
{
  char rreq_path[512];
  char local_path[512];
  // requests_left is volatile sig_atomic_t
  while (check()>0)
  {
    gfcrequest_t *gfr = NULL;
    // Create socket for the worker
    gfr = gfc_create();
    gfc_set_server(&gfr, server);
    gfc_set_port(&gfr, port);
    gfc_set_writefunc(&gfr, writecb);

    pthread_mutex_lock(&run_lock_b);
    int returncode = 0;
    while (steque_isempty(list))
    {
      // QUEUE IS EMPTY NOW
      pthread_cond_wait(&run_cond_b, &run_lock_b);
    }
    // Resume thread by popping the queue
    strcpy(rreq_path, steque_pop(list));
    pthread_cond_broadcast(&run_cond_b);
    pthread_mutex_unlock(&run_lock_b);
    // Release Mutex
    pthread_mutex_lock(&run_lock_a);
    requests_left = requests_left - 1;
    pthread_mutex_unlock(&run_lock_a);

    localPath(rreq_path, local_path);
    // Open file
    FILE *file = openFile(local_path);
    gfc_set_path(&gfr, rreq_path);
    gfc_set_writearg(&gfr, file);
    fprintf(stdout, "Requesting %s%s\n", server, rreq_path);
    if (0 > (returncode = gfc_perform(&gfr)))
    {
      fprintf(stdout, "gfc_perform returned an error %d\n", returncode);
      fclose(file);
      if (0 > unlink(local_path))
        fprintf(stderr, "warning: unlink failed on %s\n", local_path);
    }
    else
    {
      fclose(file);
    }
    if (gfc_get_status(&gfr) != GF_OK)
    {
      if (0 > unlink(local_path))
        fprintf(stderr, "warning: unlink failed on %s\n", local_path);
    }
    fprintf(stdout, "Status: %s\n", gfc_strstatus(gfc_get_status(&gfr)));
    fprintf(stdout, "Received %zu of %zu bytes\n", gfc_get_bytesreceived(&gfr),
            gfc_get_filelen(&gfr));

    gfc_cleanup(&gfr);
  }
  return 0;
}

/* Main ========================================================= */
int main(int argc, char **argv)
{
  /* COMMAND LINE OPTIONS ============================================= */
  char *workload_path = "workload.txt";
  int option_char = 0;
  int nrequests = 2;
  int nthreads = 2;
  char *req_path = NULL;
  int i = 0;
  setbuf(stdout, NULL); // disable caching
  // Parse and set command line arguments
  while ((option_char = getopt_long(argc, argv, "p:n:hs:w:r:t:", gLongOptions,
                                    NULL)) != -1)
  {
    switch (option_char)
    {
    case 'h': // help
      Usage();
      exit(0);
      break;
    case 'r':
      nrequests = atoi(optarg);
      break;
    case 'n': // nrequests
      break;
    case 'p': // port
      port = atoi(optarg);
      break;
    default:
      Usage();
      exit(1);
    case 's': // server
      server = optarg;
      break;
    case 't': // nthreads
      nthreads = atoi(optarg);
      break;
    case 'w': // workload-path
      workload_path = optarg;
      break;
    }
  }

  if (EXIT_SUCCESS != workload_init(workload_path))
  {
    fprintf(stderr, "Unable to load workload file %s.\n", workload_path);
    exit(EXIT_FAILURE);
  }
  if (nthreads < 1)
  {
    nthreads = 1;
  }
  if (nthreads > MAX_THREADS)
  {
    nthreads = MAX_THREADS;
  }
  if (nthreads > nrequests)
  {
    nthreads = nrequests;
  }
  requests_left = nrequests;
  gfc_global_init();
  /* Create threadpool */
  list = (steque_t *)malloc(sizeof(steque_t));
  steque_init(list);

  pthread_t *thread_pool;
  thread_pool = (pthread_t *)malloc(nthreads * sizeof(pthread_t));
  for (int i = 0; i < nthreads; i++)
  {
    pthread_create(&thread_pool[i], NULL, worker, (void *)&nrequests);
  }

  /* Build queue requests */

  for (i = 1; i <= nrequests; i++)
  {
    // Push to queue - Use Mutex
    pthread_mutex_lock(&run_lock_b);
    req_path = workload_get_path();
    if (strlen(req_path) > 1024)
    {
      fprintf(stderr, "Request path exceeded maximum of 1024 characters\n.");
      exit(EXIT_FAILURE);
    }
    steque_enqueue(list, req_path);
    pthread_cond_signal(&run_cond_b); //Wake Workers
    pthread_mutex_unlock(&run_lock_b);
  }
  for (int i = 0; i < nthreads; i++)
  {

    pthread_join(thread_pool[i], NULL);
  }
  free(thread_pool);
  steque_destroy(list);
  gfc_global_cleanup(); // use for any global cleanup for AFTER your thread
                        // pool has terminated.

  return 0;
}