/*
 ============================================================================
 Name        : gfclient.c
 Author      : Prateek Gupta
 Version     : v1
 ============================================================================
 */

/*                                      File Usage - Ubuntu                                     */
/*
Compile: make clean all
Execute: ./gfclient_download
Return: -
Example:
root@8c9f19e07061:/workspace/pr1/gflib# ./gfclient_download -h
usage:
  webclient [options]
options:
  -h                  Show this help message
  -n [num_requests]   Requests download per thread (Default: 2)
  -p [server_port]    Server port (Default: 20121)
  -s [server_addr]    Server address (Default: 127.0.0.1)
  -l [workload_path]  Path to workload file (Default: workload.txt)
*/

#include "gfclient-student.h"

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

// Struct for holding server socket information
struct gfcrequest_t
{
    char *server;
    char *req_path;
    unsigned short port;
    size_t bytesreceived;
    size_t filelen;
    gfstatus_t status;
    void *headerarg;
    void *writearg;
    void (*headerfunc)(void *, size_t, void *);
    void (*writefunc)(void *, size_t, void *);
};

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

// optional function for cleaup processing.
void gfc_cleanup(gfcrequest_t **gfr)
{
    free((*gfr)->req_path);
    free((*gfr)->server);
    free(*gfr);
    *gfr = NULL;
}

// Allocate memory for client struct
gfcrequest_t *gfc_create()
{
    gfcrequest_t *gfr;
    gfr = (gfcrequest_t *)malloc(sizeof(gfcrequest_t));
    gfr->bytesreceived = 0;
    gfr->filelen = 0;
    gfr->headerarg = NULL;
    gfr->writearg = NULL;
    return gfr;
}

// Get Information on bytes received
size_t gfc_get_bytesreceived(gfcrequest_t **gfr)
{
    gfcrequest_t *serv=*gfr;
    return serv->bytesreceived;
}

// Get information on file length
size_t gfc_get_filelen(gfcrequest_t **gfr)
{
    gfcrequest_t *serv=*gfr;
    return serv->filelen;
}

// Get status of request
gfstatus_t gfc_get_status(gfcrequest_t **gfr)
{
    gfcrequest_t *serv=*gfr;
    return serv->status;
}

// Initialize global variable
void gfc_global_init()
{
}

// Clear Global Variables from heap
void gfc_global_cleanup()
{
}
// Start the socket
int gfc_perform(gfcrequest_t **gfr)
{
    gfcrequest_t *serv=*gfr;
    // Helper variables with default values
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
    sprintf(PORT, "%d", serv->port);
    // Filling the socket with default values
    if ((rv = getaddrinfo(serv->server, PORT, &hints, &servinfo)) != 0)
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
        serv->status = GF_INVALID;
        freeaddrinfo(servinfo);
        return -1;
    }
    // Freeing addrinfo
    freeaddrinfo(servinfo);
    char message[BUFSIZ];
    memset(message, '\0', BUFSIZ);
    sprintf(message, "%s %s %s", "GETFILE GET", serv->req_path, "\r\n\r\n");
    int charsRead = -1;
    // Send File Request to Server
    charsRead = send(socketFD, message, strlen(message), 0);
    if (charsRead <= 0)
    {
        error("ERROR writing to socket");
        serv->status = GF_INVALID;
        close(socketFD);
        return -1;
    }
    // Start Receiving the data
    char *readBuffer;
    readBuffer = malloc(1024);
    int r = -1;
    // <scheme> <status> <length>\r\n\r\n<content>
    int total = 0;
    memset(readBuffer, '\0', 1024); // Clear the buffer
    char *start = readBuffer;
    while (strstr(start, "\r\n\r\n") == NULL) // As long as we haven't found the terminal...
    {
        r = recv(socketFD, (void *)readBuffer, 128, 0); // Get the next chunk
        if (r > 0)
        {
            readBuffer = readBuffer + r;
            total = total + r;
        }
        else
        {
            serv->status = GF_INVALID;
            free(start);
            close(socketFD);
            return -1;
        }
    }
    // Check for getfile string format
    char sscheme[128];
    char sstatus[128];
    char placeholderint[128];
    sscanf(start, "%s %s %s\r\n\r\n", sscheme, sstatus, placeholderint);
    // Update status
    if (strcmp(sstatus, "OK") == 0)
        serv->status = GF_OK;
    else if (strcmp(sstatus, "FILE_NOT_FOUND") == 0)
        serv->status = GF_FILE_NOT_FOUND;
    else if (strcmp(sstatus, "ERROR") == 0)
        serv->status = GF_ERROR;

    if (serv->status == GF_FILE_NOT_FOUND)
    {
        free(start);
        close(socketFD);
        return 0;
    }
    if (serv->status == GF_ERROR)
    {
        free(start);
        close(socketFD);
        return 0;
    }
    if (strcmp(sscheme, "GETFILE") != 0)
    {
        serv->status = GF_INVALID;
        free(start);
        close(socketFD);
        return -1;
    }
    size_t filelength = atoi(placeholderint);
    if (filelength == 0)
    {
        serv->status = GF_INVALID;
        free(start);
        close(socketFD);
        return -1;
    }
    char header[128];
    sprintf(header, "GETFILE %s %ld\r\n\r\n", sstatus, filelength);
    int header_length = strlen(header);
    char *data;
    data = start + header_length;
    int bytesreceived = total - header_length;
    printf("Total File Length is %ld\n", filelength);
    // Call Handler
    serv->writefunc((void *)data, bytesreceived, serv->writearg);
    int remaining_data = filelength - bytesreceived;
    // Start Receiving File Data
    if (serv->status == GF_OK)
    {
        r = 1;
        while ((remaining_data != 0) && (r > 0))
        {
            memset(readBuffer, '\0', 128);
            r = -1;
            r = recv(socketFD, (void *)readBuffer, 128, 0);
            if (r > 0)
            {
                bytesreceived = bytesreceived + r;
                remaining_data = remaining_data - r;
                // Write bytes received to file
                serv->writefunc((void *)readBuffer, r, serv->writearg);
                serv->bytesreceived = bytesreceived;
                serv->filelen = filelength;
            }
            else if (r < 0)
            {
                printf("ERROR: EXITING LOOP; Remaining data is %d\n", remaining_data);
                free(start);
                close(socketFD);
                return -1;
            }
            else
            {
                free(start);
                close(socketFD);
                return -1;
            }
        }
    }
    // Error Trapping
    if ((remaining_data < 0) || (serv->status != GF_OK))
    {
        free(start);
        close(socketFD);
        return -1;
    }
    // Free the socket
    free(start);
    close(socketFD);
    return 0;
}

void gfc_set_headerarg(gfcrequest_t **gfr, void *headerarg)
{
    gfcrequest_t *serv=*gfr;
    serv->headerarg = malloc(sizeof(headerarg) + 1);
    //memset to \0
    memset(serv->headerarg, '\0', sizeof(headerarg) + 1);
    serv->headerarg = headerarg;
}

void gfc_set_headerfunc(gfcrequest_t **gfr, void (*headerfunc)(void *, size_t, void *))
{
    gfcrequest_t *serv=*gfr;
    serv->headerfunc = headerfunc;
}

void gfc_set_path(gfcrequest_t **gfr, const char *path)
{
    gfcrequest_t *serv=*gfr;
    serv->req_path = (char *)malloc((strlen(path) * sizeof(char)) + 1);
    memset(serv->req_path, '\0', (strlen(path) * sizeof(char)) + 1);
    strcpy(serv->req_path, path);
}

void gfc_set_port(gfcrequest_t **gfr, unsigned short port)
{
    gfcrequest_t *serv=*gfr;
    serv->port = port;
}

void gfc_set_server(gfcrequest_t **gfr, const char *server)
{
    gfcrequest_t *serv=*gfr;
    serv->server = (char *)malloc((strlen(server) * sizeof(char)) + 1);
    //memset to \0
    memset(serv->server, '\0', (strlen(server) * sizeof(char)) + 1);
    strcpy(serv->server, server);
}

void gfc_set_writearg(gfcrequest_t **gfr, void *writearg)
{
    gfcrequest_t *serv=*gfr;
    serv->writearg = writearg;
}

void gfc_set_writefunc(gfcrequest_t **gfr, void (*writefunc)(void *, size_t, void *))
{
    gfcrequest_t *serv=*gfr;
    serv->writefunc = writefunc;
}

const char *gfc_strstatus(gfstatus_t status)
{
    const char *strstatus = NULL;

    switch (status)
    {
    default:
    {
        strstatus = "UNKNOWN";
    }
    break;

    case GF_INVALID:
    {
        strstatus = "INVALID";
    }
    break;

    case GF_FILE_NOT_FOUND:
    {
        strstatus = "FILE_NOT_FOUND";
    }
    break;

    case GF_ERROR:
    {
        strstatus = "ERROR";
    }
    break;

    case GF_OK:
    {
        strstatus = "OK";
    }
    break;
    }
    return strstatus;
}