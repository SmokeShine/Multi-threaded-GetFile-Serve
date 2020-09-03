#include "gfserver-student.h"

/* 
 * Modify this file to implement the interface specified in
 * gfserver.h.
 */

struct gfserver_t
{
    unsigned int port;
    unsigned int max_npending;
    gfh_error_t (*handler)(gfcontext_t **, const char *, void *);
    void *handlerarg;
    // gfcontext_t *request;
};

struct gfcontext_t
{
    int establishedConnectionFD;
    gfstatus_t status; //request has a status
    char path[1024];
};

//Take ctx and abort something
void gfs_abort(gfcontext_t **ctx)
{
    //one more non sense

    close((*ctx)->establishedConnectionFD);
    free((*ctx));
}

void error(const char *msg)
{
    // Error function used for reporting issues
    perror(msg);
    exit(0);
}

gfserver_t *gfserver_create()
{
    gfserver_t *gfs;
    gfs = (gfserver_t *)malloc(sizeof(gfserver_t));
    return gfs;
}

ssize_t gfs_send(gfcontext_t **ctx, const void *data, size_t len)
{
    ssize_t bytessent = send((*ctx)->establishedConnectionFD, (void *)data, len, 0);
    return bytessent;
}

ssize_t gfs_sendheader(gfcontext_t **ctx, gfstatus_t status, size_t file_len)
{
    printf("***********\n");

    printf("%d\n", (*ctx)->status);
    printf("%d\n", status);
    printf("%d\n", (*ctx)->establishedConnectionFD);
    printf("%s\n", (*ctx)->path);
    printf("***********\n");

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

    int i = send((*ctx)->establishedConnectionFD, (void *)header, strlen(header), 0);

    if (status != GF_OK)
    {
        
        (*ctx)->status = GF_INVALID;
        close((*ctx)->establishedConnectionFD);
        return -1;
    }
    return i;
}

void gfserver_serve(gfserver_t **gfs)
{
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
    sprintf(PORT, "%d", (*(*gfs)).port);

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

    freeaddrinfo(servinfo); // all done with this structure

    if (p == NULL)
    {
        fprintf(stderr, "server: failed to bind\n");
        exit(1);
    }

    listen(listenSocketFD, (*(*gfs)).max_npending); // Flip the socket on - it can now receive up to 5 connections
    while (1)
    {
        // Accept a connection, blocking if one is not available until one connects
        sin_size = sizeof their_addr;
        establishedConnectionFD = accept(listenSocketFD, (struct sockaddr *)&their_addr, &sin_size);
        gfcontext_t *gfc = (gfcontext_t *)malloc(sizeof(gfcontext_t));
        // (*gfs)->request = gfc;
        (*gfc).establishedConnectionFD = establishedConnectionFD;
        // (*gfc).status=GF_INVALID;
        if (establishedConnectionFD < 0)
            error("ERROR on accept");
        char *readBuffer;
        readBuffer = malloc(1024);
        int r = -1;
        // <scheme> <status> <length>\r\n\r\n<content>
        int total = 0;

        memset(readBuffer, '\0', 1024); // Clear the buffer

        char *start = readBuffer;
        while (strstr(start, "\r\n\r\n") == NULL) // As long as we haven't found the terminal...
        {
            r = recv(establishedConnectionFD, (void *)readBuffer, 128, 0); // Get the next chunk
            if (r > 0)
            {
                readBuffer = readBuffer + r;
                total = total + r;
                (*gfc).status = GF_OK;
            }
            else
            {
                perror("Error in receiving data\n");
                (*gfc).status = GF_ERROR;
                gfs_abort(&gfc);
                break;
            }
        }

        printf("Total Length till now:%d\n", total);
        // Check for getfile string format
        char sscheme[128];
        char mmethod[128];
        char path[1024];
        sscanf(start, "%s %s %s\r\n\r\n", sscheme, mmethod, path);

        char completeMessage[1024];
        sscanf(start, "%s %s %s\r\n\r\n", sscheme, mmethod, path);
        sprintf(completeMessage, "%s %s %s\r\n\r\n", sscheme, mmethod, path);
        printf("\n**********************\nComplete Message Received is %s\n", completeMessage);
        int send = 1;
        if (((strcmp(sscheme, "GETFILE") != 0) || (strcmp(mmethod, "GET") != 0)) != 0)
        {
            printf("Format error for scheme and method\n");
            printf("%s %s", sscheme, mmethod);
            send = 0;
        }
        printf("Path is %s\n", path);
        int filestartcheck = -1;
        filestartcheck = strncmp(path, "/", 1);
        if (filestartcheck != 0)
        {
            send = 0;
        }

        if (send == 0)
        {
            gfs_sendheader(&gfc, GF_INVALID, -1);
            gfc->status = GF_INVALID;
        }
        else
        {
            strcpy((*gfc).path, path);
            (*gfs)->handler(&gfc, (*gfc).path, (*gfs)->handlerarg);
        }
    }
}

void gfserver_set_handlerarg(gfserver_t **gfs, void *arg)
{
    //malloc
    // (*(*gfs)).handlerarg = malloc(sizeof(arg));
    //memset to \0
    // memset((*(*gfs)).handlerarg, '\0', sizeof(arg));
    (*(*gfs)).handlerarg = arg;
}

void gfserver_set_handler(gfserver_t **gfs, gfh_error_t (*handler)(gfcontext_t **, const char *, void *))
{
    //looks like handler
    (*(*gfs)).handler = handler;
}

void gfserver_set_maxpending(gfserver_t **gfs, int max_npending)
{
    (*(*gfs)).max_npending = max_npending;
}

void gfserver_set_port(gfserver_t **gfs, unsigned short port)
{
    (*(*gfs)).port = port;
}