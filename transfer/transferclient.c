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

char *message = "Hello World!!";

/*Error handlers for socket connections*/
void error(const char *msg)
{
    // Error function used for reporting issues
    perror(msg);
    exit(0);
}

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

/* Main ========================================================= */
int main(int argc, char **argv)
{
    int option_char = 0;
    char *hostname = "localhost";
    unsigned short portno = 20801;
    char *filename = "cs6200.txt";
    // char *filename = "test.txt";
    // truncate -s 10M output.file
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

    if (NULL == hostname)
    {
        fprintf(stderr, "%s @ %d: invalid host name\n", __FILE__, __LINE__);
        exit(1);
    }

    if (NULL == filename)
    {
        fprintf(stderr, "%s @ %d: invalid filename\n", __FILE__, __LINE__);
        exit(1);
    }

    if ((portno < 1025) || (portno > 65535))
    {
        fprintf(stderr, "%s @ %d: invalid port number (%d)\n", __FILE__, __LINE__, portno);
        exit(1);
    }

    /* Socket Code Here */
    int socketFD;
    struct addrinfo hints, *servinfo, *p;
    int rv;
    // char s[INET6_ADDRSTRLEN];

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    char PORT[256];
    sprintf(PORT, "%d", portno);
    if ((rv = getaddrinfo(hostname, PORT, &hints, &servinfo)) != 0)
    {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
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

    //  char s[INET6_ADDRSTRLEN];

    // inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr),
    //         s, sizeof s);
    // printf("%s---",s);
    // printf("%d---",p->ai_family);

    freeaddrinfo(servinfo); // all done with this structure

    FILE *received_file;
    received_file = fopen(filename, "w");
    if (received_file == NULL)
    {
        exit(EXIT_FAILURE);
    }
    char buffer[1024];
    int charsRead;
    memset(buffer, '\0', 1024);
    while ((charsRead = recv(socketFD, buffer, sizeof(buffer), 0)) > 0)
    {
        fwrite(buffer, sizeof(char), charsRead, received_file);
    }
    fclose(received_file);
    close(socketFD); // Close the socket
    return 0;
}
