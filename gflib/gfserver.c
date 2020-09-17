/*
 ============================================================================
 Name        : gfserver.c
 Author      : Prateek Gupta
 Version     : v1
 ============================================================================
 */

/*                                      File Usage - Ubuntu                                     */
/*
Compile: make clean all
Execute: ./gfserver_main
Return: -
Example:
root@8c9f19e07061:/workspace/pr1/gflib# ./gfserver_main -h
usage:
  gfserver_main [options]
options:
  -p [listen_port]    Listen port (Default: 20121)
  -m [content_file]   Content file mapping keys to content files
  -h                  Show this help message.
*/

#include "gfserver-student.h"

// Struct to contain information about listening socket
struct gfserver_t
{
    unsigned int port;
    unsigned int max_npending;
    gfh_error_t (*handler)(gfcontext_t **, const char *, void *);
    void *handlerarg;
    gfcontext_t *socket_context;
};

// Struct to contain information about the client connection socket
struct gfcontext_t
{
    int establishedConnectionFD;
    gfstatus_t status;
    char path[1024];
};

// Close connection to the client
void gfs_abort(gfcontext_t **ctx)
{
    close((*ctx)->establishedConnectionFD);
    free((*ctx));
    *ctx=NULL;
}

// Standard Error Trapping
void error(const char *msg)
{
    // Error function used for reporting issues
    perror(msg);
    exit(0);
}

// Allocate memory for server
gfserver_t *gfserver_create()
{
    gfserver_t *gfs;
    gfs = (gfserver_t *)malloc(sizeof(gfserver_t));
    return gfs;
}

// Send data to the client
ssize_t gfs_send(gfcontext_t **ctx, const void *data, size_t len)
{
    ssize_t bytessent = send((*ctx)->establishedConnectionFD, (void *)data, len, 0);
    return bytessent;
}

// Send header to the client
ssize_t gfs_sendheader(gfcontext_t **ctx, gfstatus_t status, size_t file_len)
{
    // Status is provided by the handler; context is provided by the server
    char *header = (char *)malloc(128);
    if (status == GF_OK)
    {
        sprintf(header, "GETFILE OK %ld\r\n\r\n", file_len);
    }
    else if (status == GF_FILE_NOT_FOUND)
    {
        strcpy(header, "GETFILE FILE_NOT_FOUND\r\n\r\n");
    }
    else if (status == GF_INVALID)
    {
        strcpy(header, "GETFILE INVALID\r\n\r\n");
    }
    else
    {
        strcpy(header, "GETFILE ERROR\r\n\r\n");
    }
    printf("Sending header to the client: %s\n", header);
    // Send header to the client
    int i = send((*ctx)->establishedConnectionFD, (void *)header, strlen(header), 0);
    if (status != GF_OK)
    {
        // Error trapping for closing connection on invalid header
        (*ctx)->status = GF_INVALID;
        close((*ctx)->establishedConnectionFD);
        return -1;
    }
    return i;
}

// Start the server
void gfserver_serve(gfserver_t **gfs)
{
    // listen on sock_fd, new connection on new_fd
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
    // Configuring TCP/UDP
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE; // use my IP
    char PORT[256];
    sprintf(PORT, "%d", (*(*gfs)).port);
    // Creating structure to contain information of service provider
    if ((rv = getaddrinfo(NULL, PORT, &hints, &servinfo)) != 0)
    {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        exit(-1);
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
    // Flip the socket on - it can now receive up to max_npending connections
    listen(listenSocketFD, (*(*gfs)).max_npending);
    while (1)
    {
        // Accept a connection, blocking if one is not available until one connects
        sin_size = sizeof their_addr;
        // Connect to client
        establishedConnectionFD = accept(listenSocketFD, (struct sockaddr *)&their_addr, &sin_size);
        gfcontext_t *gfc = (gfcontext_t *)malloc(sizeof(gfcontext_t));
        (*gfs)->socket_context = gfc;
        (*gfc).establishedConnectionFD = establishedConnectionFD;
        if (establishedConnectionFD < 0)
            error("ERROR on accept");
        char *readBuffer;
        readBuffer = malloc(1024);
        int r = -1;
        // <scheme> <status> <length>\r\n\r\n<content>
        int total = 0;
        memset(readBuffer, '\0', 1024); // Clear the buffer
        char *start = readBuffer;
        // Loop till end of header tag is found
        while (strstr(start, "\r\n\r\n") == NULL)
        {
            r = recv(establishedConnectionFD, (void *)readBuffer, 128, 0);
            if (r > 0)
            {
                readBuffer = readBuffer + r;
                total = total + r;
                gfc->status = GF_OK;
            }
            else
            {
                perror("Error in receiving data\n");
                gfc->status = GF_ERROR;
                gfs_abort(&gfc);
                break;
            }
        }
        // Check for getfile string format
        char sscheme[128];
        char mmethod[128];
        char path[1024];
        sscanf(start, "%s %s %s\r\n\r\n", sscheme, mmethod, path);
        char completeMessage[1024];
        sscanf(start, "%s %s %s\r\n\r\n", sscheme, mmethod, path);
        sprintf(completeMessage, "%s %s %s\r\n\r\n", sscheme, mmethod, path);
        // Check for format
        int send = 1;
        if (((strcmp(sscheme, "GETFILE") != 0) || (strcmp(mmethod, "GET") != 0)) != 0)
        {
            printf("Format error for scheme and method\n");
            printf("%s %s", sscheme, mmethod);
            send = 0;
        }
        int filestartcheck = -1;
        filestartcheck = strncmp(path, "/", 1);
        if (filestartcheck != 0)
        {
            send = 0;
        }
        if (send == 0)
        {
            // Valid header; allow sending data
            gfs_sendheader(&gfc, GF_INVALID, -1);
            gfc->status = GF_INVALID;
        }
        else
        {
            // Invalid header; close connection after sending header
            strcpy((*gfc).path, path);
            (*gfs)->handler(&gfc, (*gfc).path, (*gfs)->handlerarg);
        }
    }
}
// Set Handler Arguments
void gfserver_set_handlerarg(gfserver_t **gfs, void *arg)
{
    (*(*gfs)).handlerarg = arg;
}

// Set handler for the server
void gfserver_set_handler(gfserver_t **gfs, gfh_error_t (*handler)(gfcontext_t **, const char *, void *))
{
    (*(*gfs)).handler = handler;
}
// Queue length of listening socket
void gfserver_set_maxpending(gfserver_t **gfs, int max_npending)
{
    (*(*gfs)).max_npending = max_npending;
}
// Set port for the server
void gfserver_set_port(gfserver_t **gfs, unsigned short port)
{
    (*(*gfs)).port = port;
}