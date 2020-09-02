
#include "gfserver-student.h"

/* 
 * Modify this file to implement the interface specified in
 * gfserver.h.
 */


struct gfserver_t
{
    unsigned int port;
    int max_npending;
    gfh_error_t (*handler)(gfcontext_t **, const char *, void *);
    void *handlerarg;
};


struct gfcontext_t
{
    int establishedConnectionFD;
};

//Take ctx and abort something
void gfs_abort(gfcontext_t **ctx)
{
    //one more non sense
    close((*ctx)->establishedConnectionFD);
}

/*Error handlers for socket connections*/
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
    // gfserver_set_port(&gfs, 20121); opaque nonsense. Don't hardcode anything here
    // gfserver_set_maxpending(&gfs, 801);
    return gfs;
}

ssize_t gfs_send(gfcontext_t **ctx, const void *data, size_t len)
{
	ssize_t bytessent = send((*ctx)->establishedConnectionFD, (void*)data, len, 0);
	return bytessent;
}

ssize_t gfs_sendheader(gfcontext_t **ctx, gfstatus_t status, size_t file_len)
{
	char *scheme = "GETFILE";
	char str_status[15] = "";
	char *header = (char *)malloc(strlen(scheme) + sizeof(str_status) + 15 + 3);

	if (status == GF_OK){
		strcpy(str_status, "OK");
		sprintf(header, "%s %s %zu\r\n\r\n", scheme, str_status, file_len);
	}
	else if (status == GF_FILE_NOT_FOUND){
		strcpy(str_status, "FILE_NOT_FOUND");
		sprintf(header, "%s %s \r\n\r\n", scheme, str_status);
	}
	else{
		strcpy(str_status, "ERROR");
		sprintf(header, "%s %s \r\n\r\n", scheme, str_status);
	}

	printf("Header = %s\n", header);
	return send((*ctx)->establishedConnectionFD, (void*)header, strlen(header), 0);
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
        gfcontext_t *gfc=(gfcontext_t *)malloc(sizeof(gfcontext_t)); //what a waste of precious memory
        gfc->establishedConnectionFD=establishedConnectionFD;
        if (establishedConnectionFD < 0)
            error("ERROR on accept");
        char buffer[MAX_REQUEST_LEN];
        int r=-1;
        memset(buffer, '\0', MAX_REQUEST_LEN);

        char completeMessage[MAX_REQUEST_LEN], readBuffer[32];
        memset(completeMessage, '\0', sizeof(completeMessage)); // Clear the buffer
        while (strstr(completeMessage, "\r\n\r\n") == NULL) // As long as we haven't found the terminal...
        {
            memset(readBuffer, '\0', sizeof(readBuffer)); // Clear the buffer
            r = recv(establishedConnectionFD, readBuffer, sizeof(readBuffer) - 1, 0); // Get the next chunk
            strcat(completeMessage, readBuffer); // Add that chunk to what we have so far
            if (r == -1) { printf("r == -1\n"); break; } // Check for errors
            if (r == 0) { break; }
        }

        printf("Message received is %s\n", completeMessage);
        char scheme[128],method[128];
        memset(scheme,'\0',128);
        strcpy(scheme,"GETFILE");
        memset(method,'\0',128);
        strcpy(method,"GET");

        char sscheme[strlen(scheme)],mmethod[strlen(method)];
        memset(sscheme,'\0',strlen(scheme));
        memset(mmethod,'\0',strlen(method));
        char path[1024];
        memset(path,'\0',1024);

        sscanf(completeMessage, "%s %s %s\r\n\r\n" , sscheme,mmethod,path);
        if (((strcmp(sscheme,scheme)!=0) || (strcmp(sscheme,scheme)!=0))!=0)
        {
            printf("Format error for scheme and method\n");
            // Add error statement
        }
        printf("Path is %s\n",path); 
        int filestartcheck=-1;
        filestartcheck=strncmp(path,"/",1);   
        printf("File Start Check for %s is %d\n",path,filestartcheck);     

        (*gfs)->handler(&gfc,path,(*gfs)->handlerarg);

        close(establishedConnectionFD); // Close the existing socket which is connected to the client
    }
}

void gfserver_set_handlerarg(gfserver_t **gfs, void *arg)
{
    //malloc
     (*(*gfs)).handlerarg= malloc(sizeof(arg));
    //memset to \0
    memset((*(*gfs)).handlerarg,'\0',sizeof(arg));
    (*(*gfs)).handlerarg=arg;
}

void gfserver_set_handler(gfserver_t **gfs, gfh_error_t (*handler)(gfcontext_t **, const char *, void *))
{
    //looks like handler
    (*(*gfs)).handler=handler;
}

void gfserver_set_maxpending(gfserver_t **gfs, int max_npending)
{
    (*(*gfs)).max_npending = max_npending;
}

void gfserver_set_port(gfserver_t **gfs, unsigned short port)
{
    (*(*gfs)).port = port;
}
