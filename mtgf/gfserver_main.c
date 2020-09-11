
#include "gfserver-student.h"

#define USAGE                                                               \
  "usage:\n"                                                                \
  "  gfserver_main [options]\n"                                             \
  "options:\n"                                                              \
  "  -t [nthreads]       Number of threads (Default: 7)\n"                  \
  "  -p [listen_port]    Listen port (Default: 20121)\n"                    \
  "  -m [content_file]   Content file mapping keys to content files\n"      \
  "  -d [delay]          Delay in content_get, default 0, range 0-5000000 " \
  "(microseconds)\n "                                                       \
  "  -h                  Show this help message.\n"

/* OPTIONS DESCRIPTOR ====================================================== */
static struct option gLongOptions[] = {
    {"content", required_argument, NULL, 'm'},
    {"delay", required_argument, NULL, 'd'},
    {"nthreads", required_argument, NULL, 't'},
    {"port", required_argument, NULL, 'p'},
    {"help", no_argument, NULL, 'h'},
    {NULL, 0, NULL, 0}};

extern steque_t *list;

// extern int run_thread_a;
// extern pthread_mutex_t run_lock_a ;
// extern pthread_cond_t run_cond_a;

// int run_thread_a=0;
// pthread_mutex_t run_lock_a= PTHREAD_MUTEX_INITIALIZER;
// pthread_cond_t run_cond_a = PTHREAD_COND_INITIALIZER;

extern int run_thread_b;
extern pthread_mutex_t run_lock_b;
extern pthread_cond_t run_cond_b;

int run_thread_b = 0;
pthread_mutex_t run_lock_b = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t run_cond_b = PTHREAD_COND_INITIALIZER;

extern unsigned long int content_delay;

extern gfh_error_t gfs_handler(gfcontext_t **ctx, const char *path, void *arg);

static void _sig_handler(int signo)
{
  if ((SIGINT == signo) || (SIGTERM == signo))
  {
    exit(signo);
  }
}

void *worker(void *arguments)
{
  // printf("Thread ID is: %ld\n", (long) pthread_self());
  while (1)
  {
    char buffer[1024];

    /* Wait for Thread B to be runnable */
    pthread_mutex_lock(&run_lock_b);

    while (steque_isempty(list))
    {
      pthread_cond_wait(&run_cond_b, &run_lock_b);
    }

    // Lock Acquired to update queue
    steque_package *item = steque_pop(list);
    // printf("File Name Stripped is %s\n",openfile);
    // printf("Thread ID is: %ld working on %s\n", (long) pthread_self(),openfile);
    // gfcontext_t *cctx = (gfcontext_t *)malloc(sizeof(gfcontext_t*));
    // cctx=item->ctx;
    pthread_cond_signal(&run_cond_b);
    pthread_mutex_unlock(&run_lock_b);

    // Extract Data and send
    if (item == NULL)
    {
      continue;
    }
    int openfile = content_get(item->path); //the server is keeping all the files descriptors open in a dictionary
    if (openfile < 0)
    {
      gfs_sendheader(&item->ctx, GF_FILE_NOT_FOUND, 0);
      continue;
    }

    // Check for path

    // printf("Sending header\n");
    /* Sending the file contents chunk by chunk. */

    /* Get file stats */
    struct stat file_stat;
    if (fstat(openfile, &file_stat) < 0)
    {
      /*Check if there is issue in reading attributes of the file*/
      fprintf(stderr, "Error fstat --> %s", strerror(errno));
      continue;
    }
    /*Update the variable*/
    int file_size = 0;
    file_size = file_stat.st_size;
    // printf("File length is %d\n",file_size);

    gfs_sendheader(&item->ctx, GF_OK, file_size);

    int bytesRead = 0;
    int bytesSent = 0;
    int total_sent = 0;

    while (total_sent < file_size)
    {
      bytesRead = pread(openfile, buffer, 1024, total_sent);
      bytesSent = gfs_send(&item->ctx, buffer, bytesRead);
      total_sent = total_sent + bytesSent;
      memset(buffer, '\0', 1024);
    }
    
    free(item);

    // printf("Bytes Transferred = %d\n", total_sent);
  }
}

void init_threads(size_t numthreads)
{
  pthread_t thread_pool[numthreads];
  for (int i = 0; i < numthreads; i++)
  {
    pthread_create(&thread_pool[i], NULL, worker, NULL);
  }
}

/* Main ========================================================= */
int main(int argc, char **argv)
{
  int option_char = 0;
  unsigned short port = 20121;
  char *content_map = "content.txt";
  gfserver_t *gfs = NULL;
  int nthreads = 7;

  setbuf(stdout, NULL);

  if (SIG_ERR == signal(SIGINT, _sig_handler))
  {
    fprintf(stderr, "Can't catch SIGINT...exiting.\n");
    exit(EXIT_FAILURE);
  }

  if (SIG_ERR == signal(SIGTERM, _sig_handler))
  {
    fprintf(stderr, "Can't catch SIGTERM...exiting.\n");
    exit(EXIT_FAILURE);
  }

  // Parse and set command line arguments
  while ((option_char = getopt_long(argc, argv, "p:t:rhm:d:", gLongOptions,
                                    NULL)) != -1)
  {
    switch (option_char)
    {
    case 't': // nthreads
      nthreads = atoi(optarg);
      break;
    case 'h': // help
      fprintf(stdout, "%s", USAGE);
      exit(0);
      break;
    case 'p': // listen-port
      port = atoi(optarg);
      break;
    default:
      fprintf(stderr, "%s", USAGE);
      exit(1);
    case 'm': // file-path
      content_map = optarg;
      break;
    case 'd': // delay
      content_delay = (unsigned long int)atoi(optarg);
      break;
    }
  }

  /* not useful, but it ensures the initial code builds without warnings */
  if (nthreads < 1)
  {
    nthreads = 1;
  }

  if (content_delay > 5000000)
  {
    fprintf(stderr, "Content delay must be less than 5000000 (microseconds)\n");
    exit(__LINE__);
  }

  content_init(content_map);

  /* Initialize thread management */

  list = (steque_t *)malloc(sizeof(steque_t));
  steque_init(list);

  // printf("Number of threads is %d\n",nthreads);
  init_threads(nthreads);

  /*Initializing server*/
  gfs = gfserver_create();

  /*Setting options*/
  gfserver_set_port(&gfs, port);
  gfserver_set_maxpending(&gfs, 20);
  gfserver_set_handler(&gfs, gfs_handler);
  gfserver_set_handlerarg(&gfs, NULL); // doesn't have to be NULL!

  /*Loops forever*/
  gfserver_serve(&gfs);
}
