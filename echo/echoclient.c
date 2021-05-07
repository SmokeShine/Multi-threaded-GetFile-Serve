/*
 ============================================================================
 Name        : echoclient.c
 Author      : Prateek Gupta
 Version     : v1
 ============================================================================
 */

/*                                      File Usage - Ubuntu                                     */
/*
Compile: make clean all
Execute: ./echoclient
Return: Text send to server and echoed back
Example:
root@8c9f19e07061:/workspace/pr1/echo# ./echoclient -h
usage:
  echoclient [options]
options:
  -s                  Server (Default: localhost)
  -p                  Port (Default: 20121)
  -m                  Message to send to server (Default: "Hello world.")
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

#define USAGE                                                          \
  "usage:\n"                                                           \
  "  echoclient [options]\n"                                           \
  "options:\n"                                                         \
  "  -s                  Server (Default: localhost)\n"                \
  "  -p                  Port (Default: 20121)\n"                      \
  "  -m                  Message to send to server (Default: \"Hello " \
  "world.\")\n"                                                        \
  "  -h                  Show this help message\n"

/* OPTIONS DESCRIPTOR ====================================================== */
static struct option gLongOptions[] = {
    {"server", required_argument, NULL, 's'},
    {"port", required_argument, NULL, 'p'},
    {"message", required_argument, NULL, 'm'},
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

/* Main ========================================================= */
int main(int argc, char **argv)
{
  // Helper variables with default values
  int option_char = 0;
  char *hostname = "localhost";
  unsigned short portno = 20121;
  char *message = "Hello World!!";
  // Parse and set command line arguments
  while ((option_char =
              getopt_long(argc, argv, "s:p:m:hx", gLongOptions, NULL)) != -1)
  {
    switch (option_char)
    {
    case 's': // server
      hostname = optarg;
      break;
    case 'p': // listen-port
      portno = atoi(optarg);
      break;
    default:
      fprintf(stderr, "%s", USAGE);
      exit(1);
    case 'm': // message
      message = optarg;
      break;
    case 'h': // help
      fprintf(stdout, "%s", USAGE);
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
  // Check for message text 
  if (NULL == message)
  {
    fprintf(stderr, "%s @ %d: invalid message\n", __FILE__, __LINE__);
    exit(1);
  }
  // Check if server hostname is available
  if (NULL == hostname)
  {
    fprintf(stderr, "%s @ %d: invalid host name\n", __FILE__, __LINE__);
    exit(1);
  }

  /* Socket Code*/
  // Creating structure to contain information of service provider
  int socketFD;
  struct addrinfo hints, *servinfo, *p;
  int rv;
  // Clearning memory for addrinfo
  memset(&hints, 0, sizeof hints);
  // Setting IPV4/IPV6 Connectivity
  hints.ai_family = AF_UNSPEC;
  // Setting Socket Type - TCP vs UDP
  hints.ai_socktype = SOCK_STREAM;
  
  char PORT[256];
  sprintf(PORT, "%d", portno);
  // Filling the socket with default values 
  if ((rv = getaddrinfo(hostname, PORT, &hints, &servinfo)) != 0)
  {
    fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
    // Error: Return 0 for successful exit and positive value for error
    return 1;
  }

  // loop through all the results and connect to the first we can
  for (p = servinfo; p != NULL; p = p->ai_next)
  {
    if ((socketFD = socket(p->ai_family, p->ai_socktype,
                           p->ai_protocol)) == -1)
    {
      perror("client: socket");
      continue;
    }

    if (connect(socketFD, p->ai_addr, p->ai_addrlen) == -1)
    {
      close(socketFD);
      perror("client: connect");
      continue;
    }
    break;
  }

  if (p == NULL)
  {

    fprintf(stderr, "client: failed to connect\n");
    freeaddrinfo(servinfo);
    return 2;
  }
  // Freeing addrinfo 
  freeaddrinfo(servinfo); 

  int charsWritten = -1;
  // Send message to server
  charsWritten = send(socketFD, message, strlen(message), 0); // Write to the server
  if (charsWritten < 0)
    error("CLIENT: ERROR writing to socket");
  if (charsWritten < strlen(message))
  // print to stdout. This is a warning and not an error
    printf("CLIENT: WARNING: Not all data written to socket!\n");

  char buffer[256];
  int charsRead = -1;
  memset(buffer, '\0', 256);
  // Read data from the socket, leaving \0 at end for valid text
  charsRead = recv(socketFD, buffer, sizeof(buffer) - 1, 0); 
  if (charsRead < 0)
    error("CLIENT: ERROR reading from socket");
  // Print text received from the server
  printf("%s", buffer);
  // Close the socket
  close(socketFD); 
  // Successful exit of client
  return 0;
}
