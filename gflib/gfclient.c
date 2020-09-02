
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
    // function pointers declaration

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
    // gfr=NULL;
}

gfcrequest_t *gfc_create()
{
    // dummy for now - need to fill this part in
    // fill the same way as server code
    gfcrequest_t *gfr;
    gfr = (gfcrequest_t *)malloc(sizeof(gfcrequest_t));
    gfr->bytesreceived = 0;
    gfr->filelen = 0;
    gfr->headerarg = NULL;
    gfr->writearg = NULL;

    return gfr;
}

size_t gfc_get_bytesreceived(gfcrequest_t **gfr)
{
    // not yet implemented
    return (*(*gfr)).bytesreceived;
}

size_t gfc_get_filelen(gfcrequest_t **gfr)
{
    // not yet implemented
    return (*(*gfr)).filelen;
}

gfstatus_t gfc_get_status(gfcrequest_t **gfr)
{
    // not yet implemented
//   GF_OK,
//   GF_FILE_NOT_FOUND,
//   GF_ERROR,
//   GF_INVALID
    return (*(*gfr)).status;
}

void gfc_global_init()
{
}

void gfc_global_cleanup()
{
}

int gfc_perform(gfcrequest_t **gfr)
{

    int socketFD;
    struct addrinfo hints, *servinfo, *p;
    int rv;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    char PORT[256];
    sprintf(PORT, "%d", (*(*gfr)).port);
    if ((rv = getaddrinfo((*(*gfr)).server, PORT, &hints, &servinfo)) != 0)
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
        (*gfr)->status = GF_INVALID;
        freeaddrinfo(servinfo);
        return -1;
    }
    freeaddrinfo(servinfo); // all done with this structure
    char message[BUFSIZ];
    memset(message, '\0', BUFSIZ);
    sprintf(message, "%s %s %s", "GETFILE GET", (*(*gfr)).req_path, "\r\n\r\n");
    printf("Message is %s\n", message);
    int charsRead = -1;
    charsRead = send(socketFD, message, strlen(message), 0); // Send File Request to Server
    if (charsRead <= 0)
    {
        error("ERROR writing to socket");
        (*gfr)->status = GF_INVALID;
        close(socketFD);
        return -1;
    }
    // else if (charsRead == 0)
    // {
    //     (*gfr)->status = GF_FILE_NOT_FOUND;
    //     close(socketFD);
    //     return -1;
    // }
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
            (*gfr)->status = GF_INVALID;
            free(start);
            close(socketFD);
            return -1;
        }
    }

    printf("Total Length till now:%d\n", total);
    // Check for getfile string format
    char sscheme[128];
    char sstatus[128];
    char placeholderint[128];
    sscanf(start, "%s %s %s\r\n\r\n", sscheme,sstatus, placeholderint);
    if (strcmp(sstatus, "OK") == 0)
        (*gfr)->status = GF_OK;
    else if (strcmp(sstatus, "FILE_NOT_FOUND") == 0)
        (*gfr)->status = GF_FILE_NOT_FOUND;
    else if (strcmp(sstatus, "ERROR") == 0)
        (*gfr)->status = GF_ERROR;

   if ((*(*gfr)).status == GF_FILE_NOT_FOUND)
    {
        free(start);
        close(socketFD);
        return 0;
    }

   if ((*(*gfr)).status == GF_ERROR)
    {
        free(start);
        close(socketFD);
        return 0;
    }
   if (strcmp(sscheme, "GETFILE") != 0)
    {
        (*(*gfr)).status=GF_INVALID;
        free(start);
        close(socketFD);
        return -1;
    }
    size_t filelength = atoi(placeholderint);
   if (filelength == 0)
    {
        (*(*gfr)).status=GF_INVALID;
        free(start);
        close(socketFD);
        return -1;
    }
    char header[128];
    sprintf(header, "GETFILE %s %ld\r\n\r\n", sstatus, filelength);
    printf("%s\n", header);
    int header_length = strlen(header);

    char *data;
    data = start + header_length;
    int bytesreceived = total - header_length;
    printf("Partial Data Received is %d\n", bytesreceived);
    printf("Total File Length is %ld\n", filelength);
    (*gfr)->writefunc((void *)data, bytesreceived, (*gfr)->writearg);
    
    int remaining_data = filelength - bytesreceived;
    int i=0;
    if ((*gfr)->status == GF_OK)
    {
        r=1;
        while ((remaining_data != 0)&&(r>0))
        {
            memset(readBuffer, '\0', 128); 
            r=-1;
            // printf("Break now -%d\n",r);
            r = recv(socketFD, (void *)readBuffer, 128, 0);
            if (r > 0)
            {
                bytesreceived = bytesreceived + r;
                remaining_data = remaining_data - r;
                (*gfr)->writefunc((void *)readBuffer, r, (*gfr)->writearg);
                // Clear the buffer
                //Updating
                
                // printf("Break now -%d\n",r);
                // sleep(2);
                // (*(*gfr)).status = GF_OK; //if file is sent from the server
                (*(*gfr)).bytesreceived = bytesreceived;
                (*(*gfr)).filelen = filelength;
                if(i%1000==0)
                {
                    printf("%d\tTotal Bytes Received: %d\n",i,bytesreceived);
                    printf("%d\tTotal Bytes Remaining: %d\n",i,remaining_data);
                }
                i++;
            }
            else if (r < 0)
            {
                // (*(*gfr)).status = GF_INVALID;
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
    
    if ((remaining_data < 0) || (*(*gfr)).status != GF_OK)
    {
        free(start);
        close(socketFD);
        return -1;
    }

    printf("\n********************\n%d\tTotal Bytes Received: %d\n",i,bytesreceived);
    printf("%d\tTotal Bytes Remaining: %d\n********************\n",i,remaining_data);

    free(start);
    close(socketFD);
    return 0;
}

void gfc_set_headerarg(gfcrequest_t **gfr, void *headerarg)
{
    // another pointer
    // void - cannot assume strlen will work
    (*(*gfr)).headerarg = malloc(sizeof(headerarg) + 1);
    //memset to \0
    memset((*(*gfr)).headerarg, '\0', sizeof(headerarg) + 1);
    (*(*gfr)).headerarg = headerarg;
}

void gfc_set_headerfunc(gfcrequest_t **gfr, void (*headerfunc)(void *, size_t, void *))
{
    //pointer - need to add \r\n\r\n end of header
    //there is a function pointer as argument
    (*gfr)->headerfunc = headerfunc;
}

void gfc_set_path(gfcrequest_t **gfr, const char *path)
{
    // pointer - put into heap
    (*(*gfr)).req_path = (char *)malloc((strlen(path) * sizeof(char)) + 1);
    //memset to \0
    memset((*(*gfr)).req_path, '\0', (strlen(path) * sizeof(char)) + 1);
    strcpy((*(*gfr)).req_path, path);
}

void gfc_set_port(gfcrequest_t **gfr, unsigned short port)
{
    (*(*gfr)).port = port;
}

void gfc_set_server(gfcrequest_t **gfr, const char *server)
{
    // pointer - put into heap
    (*(*gfr)).server = (char *)malloc((strlen(server) * sizeof(char)) + 1);
    //memset to \0
    memset((*(*gfr)).server, '\0', (strlen(server) * sizeof(char)) + 1);
    strcpy((*(*gfr)).server, server);
}

void gfc_set_writearg(gfcrequest_t **gfr, void *writearg)
{
    (*(*gfr)).writearg = writearg;
}

void gfc_set_writefunc(gfcrequest_t **gfr, void (*writefunc)(void *, size_t, void *))
{
    (*(*gfr)).writefunc = writefunc;
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
