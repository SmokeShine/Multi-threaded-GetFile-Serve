/*
 ============================================================================
 Name        : transferserver.c
 Author      : Prateek Gupta
 Version     : v1
 ============================================================================
 */

/*                                      File Usage - Ubuntu                                     */
/*
Compile: make clean all
Execute: ./transferserver
Return: -
Example:
root@8c9f19e07061:/workspace/pr1/transfer# ./transferserver -h
usage:
  transferserver [options]
options:
  -f                  Filename (Default: 6200.txt)
  -h                  Show this help message
  -p                  Port (Default: 20801)*/
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

#define USAGE                                              \
    "usage:\n"                                             \
    "  transferserver [options]\n"                         \
    "options:\n"                                           \
    "  -f                  Filename (Default: 6200.txt)\n" \
    "  -h                  Show this help message\n"       \
    "  -p                  Port (Default: 20801)\n"

/* OPTIONS DESCRIPTOR ====================================================== */
static struct option gLongOptions[] = {
    {"filename", required_argument, NULL, 'f'},
    {"help", no_argument, NULL, 'h'},
    {"port", required_argument, NULL, 'p'},
    {NULL, 0, NULL, 0}};

/*Error handlers for socket connections*/
void error(const char *msg)
{
    // Error function used for reporting issues
    perror(msg);
    exit(0);
}

int main(int argc, char **argv)
{
    // Helper variables with default values
    int option_char;
    int portno = 20801;          /* port to listen on */
    char *filename = "6200.txt"; /* file to transfer */

    setbuf(stdout, NULL); // disable buffering

    // Parse and set command line arguments
    while ((option_char = getopt_long(argc, argv, "xp:hf:", gLongOptions, NULL)) != -1)
    {
        switch (option_char)
        {
        case 'p': // listen-port
            portno = atoi(optarg);
            break;
        default:
            fprintf(stderr, "%s", USAGE);
            exit(1);
        case 'h': // help
            fprintf(stdout, "%s", USAGE);
            exit(0);
            break;
        case 'f': // file to transfer
            filename = optarg;
            break;
        }
    }
    //  Check portnumber
    if ((portno < 1025) || (portno > 65535))
    {
        fprintf(stderr, "%s @ %d: invalid port number (%d)\n", __FILE__, __LINE__, portno);
        exit(1);
    }
    // Check for filename
    if (NULL == filename)
    {
        fprintf(stderr, "%s @ %d: invalid filename\n", __FILE__, __LINE__);
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
    // Flip the socket on - it can now receive up to 5 connections
    listen(listenSocketFD, 5);
    while (1)
    {
        // Accept a connection, blocking if one is not available until one connects
        sin_size = sizeof their_addr;
        establishedConnectionFD = accept(listenSocketFD, (struct sockaddr *)&their_addr, &sin_size);

        if (establishedConnectionFD < 0)
            error("ERROR on accept");
        // Open the file
        FILE *fd;
        fd = fopen(filename, "r");
        int sent_bytes = -1;
        int total_sent = 0;
        int bytesRead = -1;
        char buffer[128];
        memset(buffer, '\0', 128);
        // Read Data From File
        while ((bytesRead = fread(buffer, 1, sizeof(buffer), fd)) > 0)
        {
            // Send Data to client
            sent_bytes = send(establishedConnectionFD, buffer, bytesRead, 0);
            total_sent = sent_bytes + total_sent;
        }
        // Close file
        fclose(fd);
        // Close socket
        close(establishedConnectionFD);
    }
}
