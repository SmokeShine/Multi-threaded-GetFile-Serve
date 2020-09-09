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

extern int run_thread_b;
extern pthread_mutex_t run_lock_b;
extern pthread_cond_t run_cond_b;

int run_thread_b = 0;
pthread_mutex_t run_lock_b = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t run_cond_b = PTHREAD_COND_INITIALIZER;

steque_t *list;
volatile int requests_left = -1;

typedef struct steque_package
{
  char *server;
  char *req_path;
  unsigned int port;
} steque_package;

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

void *worker(void *arguments)
{

  while (requests_left > 0)
  {

    pthread_mutex_lock(&run_lock_b);
    while (run_thread_b == 0)
    {
      pthread_cond_wait(&run_cond_b, &run_lock_b);
    }

    // We have the lock now
    steque_package *item = steque_pop(list);
    requests_left = requests_left - 1;
    printf("***************************\n");
    printf("Number of requests left is %d\n", requests_left);
    printf("Current Queue Size after popping is %d\n", steque_size(list));
    printf("%d \t %s \t %s \n", requests_left, item->server, item->req_path);
    printf("***************************\n");

    run_thread_b = steque_size(list);
    pthread_cond_signal(&run_cond_b);
    pthread_mutex_unlock(&run_lock_b);

    char local_path[512];
    memset(local_path, '\0', 512);
    localPath(item->req_path, local_path);
    FILE *file = openFile(local_path);
    gfcrequest_t *gfr = NULL;
    gfr = gfc_create();

    gfc_set_server(&gfr, item->server);
    gfc_set_path(&gfr, item->req_path);
    gfc_set_port(&gfr, item->port);
    gfc_set_writefunc(&gfr, writecb);
    gfc_set_writearg(&gfr, file);
    fprintf(stdout, "Requesting %s%s\n", item->server, item->req_path);
    int returncode = 0;
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
  char *server = "localhost";
  unsigned short port = 20121;
  char *workload_path = "workload.txt";
  int option_char = 0;
  int nrequests = 2;
  int nthreads = 2;
  // int returncode = 0;
  // gfcrequest_t *gfr = NULL;
  // FILE *file = NULL;
  char *req_path = NULL;
  // char local_path[1066];
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

  gfc_global_init();

  /* add your threadpool creation here */
  list = (steque_t *)malloc(sizeof(steque_t));
  steque_init(list);
  printf("Current length of list is %d\n", steque_size(list));

  printf("Number of threads is %d\n", nthreads);

  pthread_t thread_pool[nthreads];
  for (int i = 0; i < nthreads; i++)
  {
    pthread_create(&thread_pool[i], NULL, worker, NULL);
  }

  /* Build your queue of requests here */
  requests_left = nrequests;
  printf("Number of requests left is %d\n", requests_left);
  steque_package *package;  
  package = (steque_package *)malloc(sizeof(steque_package));

  for (i = 0; i < nrequests; i++)
  {
    printf("Current Request is %d\n", i);
    req_path = workload_get_path();
    printf("Req Path is %s\n", req_path);
    if (strlen(req_path) > 1024)
    {
      fprintf(stderr, "Request path exceeded maximum of 1024 characters\n.");
      exit(EXIT_FAILURE);
    }

    // Push to queue
    pthread_mutex_lock(&run_lock_b);
    memset(package,'\0',sizeof(steque_package));
    package->port = port;
    printf("Port is %d\n", package->port);
    package->req_path = req_path;
    package->server = server;
    steque_enqueue(list, package);
    
    printf("Current length of list is %d\n", steque_size(list));
    
    run_thread_b = steque_size(list);
    pthread_cond_signal(&run_cond_b); 
    pthread_mutex_unlock(&run_lock_b);
  }
  
  for (int i = 0; i < nthreads; i++)
  {
    pthread_join(thread_pool[i], NULL);
  }
  free(list);
  free(package);
  gfc_global_cleanup(); // use for any global cleanup for AFTER your thread
                        // pool has terminated.

  return 0;
}
