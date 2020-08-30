#include <getopt.h>
#include <netdb.h>
#include <netinet/in.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <arpa/inet.h>

#define BUFSIZE 2012

#define USAGE                                                        \
  "usage:\n"                                                         \
  "  echoserver [options]\n"                                         \
  "options:\n"                                                       \
  "  -p                  Port (Default: 20121)\n"                    \
  "  -m                  Maximum pending connections (default: 1)\n" \
  "  -h                  Show this help message\n"

/* OPTIONS DESCRIPTOR ====================================================== */
static struct option gLongOptions[] = {
    {"port", required_argument, NULL, 'p'},
    {"maxnpending", required_argument, NULL, 'm'},
    {"help", no_argument, NULL, 'h'},
    {NULL, 0, NULL, 0}};

/*Error handlers for socket connections*/
void error(const char *msg)
{
  // Error function used for reporting issues
  perror(msg);
  exit(0);
}
// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
  if (sa->sa_family == AF_INET)
  {
    return &(((struct sockaddr_in *)sa)->sin_addr);
  }

  return &(((struct sockaddr_in6 *)sa)->sin6_addr);
}

int main(int argc, char **argv)
{
  int option_char;
  int portno = 20121; /* port to listen on */
  int maxnpending = 1;

  // Parse and set command line arguments
  while ((option_char =
              getopt_long(argc, argv, "p:m:hx", gLongOptions, NULL)) != -1)
  {
    switch (option_char)
    {
    case 'p': // listen-port
      portno = atoi(optarg);
      break;
    default:
      fprintf(stderr, "%s ", USAGE);
      exit(1);
    case 'm': // server
      maxnpending = atoi(optarg);
      break;
    case 'h': // help
      fprintf(stdout, "%s ", USAGE);
      exit(0);
      break;
    }
  }

  setbuf(stdout, NULL); // disable buffering

  if ((portno < 1025) || (portno > 65535))
  {
    fprintf(stderr, "%s @ %d: invalid port number (%d)\n", __FILE__, __LINE__,
            portno);
    exit(1);
  }
  if (maxnpending < 1)
  {
    fprintf(stderr, "%s @ %d: invalid pending count (%d)\n", __FILE__, __LINE__,
            maxnpending);
    exit(1);
  }

  /* Socket Code Here */

  int listenSocketFD, establishedConnectionFD; // listen on sock_fd, new connection on new_fd
  struct addrinfo hints, *servinfo, *p;
  struct sockaddr_storage their_addr; // connector's address information
  socklen_t sin_size;
  int yes = 1;
  // char s[INET6_ADDRSTRLEN];
  int rv;
  char buffer[256];
  int charsRead = -1;
  memset(&hints, 0, sizeof hints);
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_PASSIVE; // use my IP
  char PORT[256];
  sprintf(PORT, "%d", portno);

  if ((rv = getaddrinfo(NULL, PORT, &hints, &servinfo)) != 0)
  {
    fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
    return 1;
  }

  // loop through all the results and bind to the first we can
  for (p = servinfo; p != NULL; p = p->ai_next)
  {
    if ((listenSocketFD = socket(p->ai_family, p->ai_socktype,
                                 p->ai_protocol)) == -1)
    {
      perror("server: socket");
      continue;
    }

    if (setsockopt(listenSocketFD, SOL_SOCKET, SO_REUSEADDR, &yes,
                   sizeof(int)) == -1)
    {
      perror("setsockopt");
      exit(1);
    }

    if (bind(listenSocketFD, p->ai_addr, p->ai_addrlen) == -1)
    {
      close(listenSocketFD);
      perror("server: bind");
      continue;
    }

    break;
  }

  freeaddrinfo(servinfo); // all done with this structure

  if (p == NULL)
  {
    fprintf(stderr, "server: failed to bind\n");
    exit(1);
  }

  listen(listenSocketFD, 5); // Flip the socket on - it can now receive up to 5 connections
  while (1)
  {
    // Accept a connection, blocking if one is not available until one connects
    sin_size = sizeof their_addr;
    establishedConnectionFD = accept(listenSocketFD, (struct sockaddr *)&their_addr, &sin_size);

    if (establishedConnectionFD < 0)
      error("ERROR on accept");

    // Get the message from the client and display it
    memset(buffer, '\0', 256);
    charsRead = recv(establishedConnectionFD, buffer, 255, 0); // Read the client's message from the socket
    if (charsRead < 0)
      error("ERROR reading from socket");
    // printf("%s", buffer);

    // Send a Success message back to the client
    charsRead = send(establishedConnectionFD, buffer, sizeof(buffer), 0); // Send success back
    if (charsRead < 0)
      error("ERROR writing to socket");
    close(establishedConnectionFD); // Close the existing socket which is connected to the client
  }
}
