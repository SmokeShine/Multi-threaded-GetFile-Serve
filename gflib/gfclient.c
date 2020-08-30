
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
    char *server ;
    char *req_path;
    unsigned short port;

    // gfc_set_server(&gfr, server);
    // gfc_set_path(&gfr, req_path);
    // gfc_set_port(&gfr, port);
    
    // gfc_set_writefunc(&gfr, writecb);
    // gfc_set_writearg(&gfr, file);
    size_t bytesreceived;
    size_t filelen;
    gfstatus_t status;
    void *headerarg;
    void *writearg;
    // function pointers declaration
    void (*writecb)(void *data, size_t data_len, void *arg);
    void (*headerfunc)(void*, size_t, void *);
    void (*writefunc)(void*, size_t, void *);
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
void gfc_cleanup(gfcrequest_t **gfr){
    free((*gfr)->server);
    free((*gfr)->writearg);
    free((*gfr)->req_path);
    free(*gfr);
}

gfcrequest_t *gfc_create(){
    // dummy for now - need to fill this part in
    // fill the same way as server code
    gfcrequest_t *gfr;
    gfr = (gfcrequest_t *)malloc(sizeof(gfcrequest_t));
    // gfc_set_server(&gfr, "localhost"); opaque nonsense. Don't hardcode anything here
    gfr->bytesreceived=-1;
    gfr->filelen=-1;
    gfr->headerarg=NULL;
    gfr->writearg=NULL;
    
    return gfr;
}

size_t gfc_get_bytesreceived(gfcrequest_t **gfr){
    // not yet implemented
    return (*(*gfr)).bytesreceived;
}

size_t gfc_get_filelen(gfcrequest_t **gfr){
    // not yet implemented
    return (*(*gfr)).filelen;
}

gfstatus_t gfc_get_status(gfcrequest_t **gfr){
    // not yet implemented
    return (*(*gfr)).status;
}

void gfc_global_init(){
}

void gfc_global_cleanup(){
}

int gfc_perform(gfcrequest_t **gfr){
    // currently not implemented.  You fill this part in.
    // this is function for socket - create accept and receive?
    // I have port and server address. enough to connect to server socket
    // need to send that fancy string  with \r\n

    int socketFD;
    struct addrinfo hints, *servinfo, *p;
    int rv;
    // char s[INET6_ADDRSTRLEN];

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
        freeaddrinfo(servinfo);
        return 2;
    }
   printf("******%s ******\n\n %s  \n\n",PORT,(*(*gfr)).server);

     char s[INET6_ADDRSTRLEN];

    inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr),
            s, sizeof s);
    printf("%s---",s);
    printf("%d---",p->ai_family);

    freeaddrinfo(servinfo); // all done with this structure
    
    // Create the request string to be send to the server
    char message[BUFSIZ];
    memset(message,'\0',BUFSIZ);
    sprintf(message, "%s %s %s", "GETFILE GET", (*(*gfr)).req_path,"\r\n\r\nPLACEHOLDER");
    printf("Message is %s-----------\n",message);
    int charsRead=-1; 
    charsRead = send(socketFD, message, strlen(message), 0); // Send success back
    if (charsRead < 0)
      error("ERROR writing to socket");
    

    // Start Receiving the data
    // may or may not contain header
    // <scheme> <status> <length>\r\n\r\n<content>
    // may be writing part is handler work? will never know 
    // cannot use strlen this time - be careful 
    // fixed size request
    // convert to ascii? lol

    char completeMessage[512], readBuffer[128];
    memset(completeMessage, '\0', 512);
    memset(readBuffer, '\0', 128); // Clear the buffer

    // you are receiving bytes because of void - it is still a string without \0
    // Do byte level comparision? 
    // check beej guide
    //Brute Force - Check for end of header flag. Block transfer data till then. 
    // After finding the header flag, check back for format
    int r=-1;
    while (strstr(completeMessage, "\r\n\r\n") == NULL) // As long as we haven't found the terminal...
    {
        memset(readBuffer, '\0', sizeof(readBuffer)); // Clear the buffer
        r = recv(socketFD, readBuffer, sizeof(readBuffer) - 1, 0); // Get the next chunk
        strcat(completeMessage, readBuffer); // Add that chunk to what we have so far
        // printf("PARENT: Message received from child: \"%s\", total: \"%s\"\n", readBuffer, completeMessage);
        if (r == -1) { printf("r == -1\n"); break; } // Check for errors
        if (r == 0) { break; }
    }
    //buffer may still have some data

        // memcopy, then check for header end - how to loop?
        // there is length field - but in bytes. how to check if response is getfile get again?
    //second parameter is size. third parameter is a function 
    //leap of faith. match with writecb - nonsense - what happens to the bytes read?
    // (*gfr)->writefunc((void *)NULL ,NULL , (*gfr)->writearg);

    close(socketFD);
    (*(*gfr)).status=GF_OK;//if file is sent from the server
    return 0;
}

void gfc_set_headerarg(gfcrequest_t **gfr, void *headerarg){
    // another pointer
    // void - cannot assume strlen will work
    (*(*gfr)).headerarg= malloc(sizeof(headerarg)+1);
    //memset to \0
    memset((*(*gfr)).headerarg,'\0',sizeof(headerarg)+1);
    (*(*gfr)).headerarg=headerarg;
}

void gfc_set_headerfunc(gfcrequest_t **gfr, void (*headerfunc)(void*, size_t, void *)){
    //pointer - need to add \r\n\r\n end of header
    //there is a function pointer as argument
    (*gfr)->headerfunc=headerfunc;
}

void gfc_set_path(gfcrequest_t **gfr, const char* path){
    // pointer - put into heap
    (*(*gfr)).req_path= (char*)malloc((strlen(path)*sizeof(char))+1);
    //memset to \0
    memset((*(*gfr)).req_path,'\0',(strlen(path)*sizeof(char))+1);
    strcpy((*(*gfr)).req_path,path) ;  
}

void gfc_set_port(gfcrequest_t **gfr, unsigned short port){
    (*(*gfr)).port = port;
}

void gfc_set_server(gfcrequest_t **gfr, const char* server){
    // pointer - put into heap
    (*(*gfr)).server= (char*)malloc((strlen(server)*sizeof(char))+1);
    //memset to \0
    memset((*(*gfr)).server,'\0',(strlen(server)*sizeof(char))+1);
    strcpy((*(*gfr)).server,server) ;  
}

void gfc_set_writearg(gfcrequest_t **gfr, void *writearg){
    (*(*gfr)).writearg= malloc(sizeof(writearg)+1);
    //memset to \0
    memset((*(*gfr)).writearg,'\0',sizeof(writearg)+1);
    // strcpy may not work
    // (*(*gfr)).writearg=writearg;
}

void gfc_set_writefunc(gfcrequest_t **gfr, void (*writefunc)(void*, size_t, void *)){
    // function pointer is an argument - looks similar to writecb
    (*(*gfr)).writefunc=writefunc;
}

const char* gfc_strstatus(gfstatus_t status){
    const char *strstatus = NULL;

    switch (status)
    {
        default: {
            strstatus = "UNKNOWN";
        }
        break;

        case GF_INVALID: {
            strstatus = "INVALID";
        }
        break;

        case GF_FILE_NOT_FOUND: {
            strstatus = "FILE_NOT_FOUND";
        }
        break;

        case GF_ERROR: {
            strstatus = "ERROR";
        }
        break;

        case GF_OK: {
            strstatus = "OK";
        }
        break;
        
    }

    return strstatus;
}

