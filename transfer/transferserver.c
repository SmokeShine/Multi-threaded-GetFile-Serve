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

/*Error handlers for socket connections*/
void error(const char *msg)
{
    // Error function used for reporting issues
    perror(msg);
    exit(0);
}

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

int main(int argc, char **argv)
{
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

    if ((portno < 1025) || (portno > 65535))
    {
        fprintf(stderr, "%s @ %d: invalid port number (%d)\n", __FILE__, __LINE__, portno);
        exit(1);
    }

    if (NULL == filename)
    {
        fprintf(stderr, "%s @ %d: invalid filename\n", __FILE__, __LINE__);
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

        // /* Step 6: Know the filesize of encrypted file*/
        // struct stat file_stat;
        FILE *fd;
        fd = fopen(filename, "r");
        // char file_size[512];
        // int remain_data = -1;
        // ssize_t len;
        // fd = open(filename, O_RDONLY);
        // if (fd == -1)
        // {
        //     fprintf(stderr, "Error opening file --> %s", strerror(errno));

        //     exit(EXIT_FAILURE);
        // }

        // /* Get file stats */

        // if (fstat(fd, &file_stat) < 0)
        // {
        //     fprintf(stderr, "Error fstat --> %s", strerror(errno));

        //     exit(EXIT_FAILURE);
        // }
        // remain_data = file_stat.st_size;
        /* Sending file data */
        int sent_bytes = -1;
        int total_sent = 0;
        int bytesRead = -1;
        char buffer[128];
        memset(buffer, '\0', 128);
        while ((bytesRead = fread(buffer, 1, sizeof(buffer), fd)) > 0)
        {
            sent_bytes = send(establishedConnectionFD, buffer, bytesRead, 0);
            total_sent = sent_bytes + total_sent;
        }
        fclose(fd); // last minute change
        close(establishedConnectionFD); // Close the existing socket which is connected to the client
    }
}
