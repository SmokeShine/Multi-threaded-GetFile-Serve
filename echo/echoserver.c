/*
 ============================================================================
 Name        : echoserver.c
 Author      : Prateek Gupta
 Version     : v1
 ============================================================================
 */

/*                                      File Usage - Ubuntu                                     */
/*
Compile: make clean all
Execute: ./echoserver
Return: -
Example:
root@8c9f19e07061:/workspace/pr1/echo# ./echoserver -h
usage:
  echoserver [options]
options:
  -p                  Port (Default: 20121)
  -m                  Maximum pending connections (default: 1)
  -h                  Show this help message
*/

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

/* A buffer large enough to contain the longest allowed string */
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
  // Check if socket belongs to ipv4 family
  if (sa->sa_family == AF_INET)
  {
    return &(((struct sockaddr_in *)sa)->sin_addr);
  }
  // Check if socket belongs to ipv6 family
  return &(((struct sockaddr_in6 *)sa)->sin6_addr);
}

int main(int argc, char **argv)
{
  // Helper variables with default values
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
  // Check for port number
  if ((portno < 1025) || (portno > 65535))
  {
    fprintf(stderr, "%s @ %d: invalid port number (%d)\n", __FILE__, __LINE__,
            portno);
    exit(1);
  }
  // Queue length for listening socket
  if (maxnpending < 1)
  {
    fprintf(stderr, "%s @ %d: invalid pending count (%d)\n", __FILE__, __LINE__,
            maxnpending);
    exit(1);
  }

  /* Socket Code Here */
  // Creating structure to contain information of service provider
  // listen on sock_fd, new connection on establishedconnectionfd
  int listenSocketFD, establishedConnectionFD; 
  struct addrinfo hints, *servinfo, *p;
  // connector's address information
  struct sockaddr_storage their_addr; 
  socklen_t sin_size;
  int yes = 1;
  int rv;
  char buffer[256];
  int charsRead = -1;
  memset(&hints, 0, sizeof hints);
  // Configuring IPV4/IPV6 connectivity
  hints.ai_family = AF_UNSPEC;
  // Configuring TCP/UDP Socket
  hints.ai_socktype = SOCK_STREAM;
  // use my IP
  hints.ai_flags = AI_PASSIVE; 
  char PORT[256];
  sprintf(PORT, "%d", portno);
  // Filling socket with default information
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
  // all done with this structure
  freeaddrinfo(servinfo); 

  if (p == NULL)
  {
    fprintf(stderr, "server: failed to bind\n");
    exit(1);
  }
  // Flip the socket on - it can now receive up to maxnpending connections
  listen(listenSocketFD, maxnpending); 
  while (1)
  {
    // Accept a connection, blocking if one is not available until one connects
    sin_size = sizeof their_addr;
    establishedConnectionFD = accept(listenSocketFD, (struct sockaddr *)&their_addr, &sin_size);
    // Error in passing connect socket to client
    if (establishedConnectionFD < 0)
      error("ERROR on accept");

    // Get the message from the client and display it
    memset(buffer, '\0', 256);
    // Read the client's message from the socket
    charsRead = recv(establishedConnectionFD, buffer, 255, 0); 
    if (charsRead < 0)
      error("ERROR reading from socket");

    // Send a Success message back to the client
    charsRead = send(establishedConnectionFD, buffer, sizeof(buffer), 0); // Send success back
    if (charsRead < 0)
      error("ERROR writing to socket");
    // Close the existing socket which is connected to the client
    close(establishedConnectionFD); 
  }
  // Never return
}
