/*
 ============================================================================
 Name        : transferclient.c
 Author      : Prateek Gupta
 Version     : v1
 ============================================================================
 */

/*                                      File Usage - Ubuntu                                     */
/*
Compile: make clean all
Execute: ./transferclient
Return: Text send to server and echoed back
Example:
root@8c9f19e07061:/workspace/pr1/echo# ./transferclient -h
usage:
  transferclient [options]
options:
  -s                  Server (Default: localhost)
  -p                  Port (Default: 20801)
  -o                  Output file (Default cs6200.txt)
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

#define BUFSIZE 820

#define USAGE                                                  \
    "usage:\n"                                                 \
    "  transferclient [options]\n"                             \
    "options:\n"                                               \
    "  -s                  Server (Default: localhost)\n"      \
    "  -p                  Port (Default: 20801)\n"            \
    "  -o                  Output file (Default cs6200.txt)\n" \
    "  -h                  Show this help message\n"

/* OPTIONS DESCRIPTOR ====================================================== */
static struct option gLongOptions[] = {
    {"server", required_argument, NULL, 's'},
    {"port", required_argument, NULL, 'p'},
    {"output", required_argument, NULL, 'o'},
    {"help", no_argument, NULL, 'h'},
    {NULL, 0, NULL, 0}};

char *message = "Hello World!!";

/*Error handlers for socket connections*/
void error(const char *msg)
{
    // Error function used for reporting issues
    perror(msg);
    exit(0);
}

/* Main ========================================================= */
int main(int argc, char **argv)
{
    // Helper variables with default values
    int option_char = 0;
    char *hostname = "localhost";
    unsigned short portno = 20801;
    char *filename = "cs6200.txt";
    setbuf(stdout, NULL);

    // Parse and set command line arguments
    while ((option_char = getopt_long(argc, argv, "s:xp:o:h", gLongOptions, NULL)) != -1)
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
        case 'o': // filename
            filename = optarg;
            break;
        case 'h': // help
            fprintf(stdout, "%s", USAGE);
            exit(0);
            break;
        }
    }
    // Check for server address
    if (NULL == hostname)
    {
        fprintf(stderr, "%s @ %d: invalid host name\n", __FILE__, __LINE__);
        exit(1);
    }
    // Check for filename
    if (NULL == filename)
    {
        fprintf(stderr, "%s @ %d: invalid filename\n", __FILE__, __LINE__);
        exit(1);
    }
    // Check for port number
    if ((portno < 1025) || (portno > 65535))
    {
        fprintf(stderr, "%s @ %d: invalid port number (%d)\n", __FILE__, __LINE__, portno);
        exit(1);
    }

    /* Socket Code Here */
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
    // Opening File
    FILE *received_file;
    received_file = fopen(filename, "w");
    if (received_file == NULL)
    {
        exit(EXIT_FAILURE);
    }
    char buffer[1024];
    int charsRead;
    memset(buffer, '\0', 1024);
    // Start writing data to buffer
    while ((charsRead = recv(socketFD, buffer, sizeof(buffer), 0)) > 0)
    {
        // Write Data to File
        fwrite(buffer, sizeof(char), charsRead, received_file);
    }
    // Close the file
    fclose(received_file);
    // Close the socket
    close(socketFD); 
    return 0;
}
