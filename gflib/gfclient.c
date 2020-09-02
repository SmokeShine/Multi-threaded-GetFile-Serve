
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

    size_t bytesreceived;
    size_t filelen;
    gfstatus_t status;
    void *headerarg;
    void *writearg;
    // function pointers declaration
    
    void (*headerfunc)(void*, size_t, void *);
    void (*writefunc)(void*, size_t, void *);

    size_t file_size;
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
    gfr->bytesreceived=0;
    gfr->filelen=0;
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

int gfc_performx(gfcrequest_t **gfr)
{

	int sockfd = 0; //socket file descriptor
	// const char *line_ending = "\r\n\r\n";
	// const char *line_beg = "GETFILE GET";
	char header[1000] = "";
	int bytes_read = 0;
	int total_bytes_read = 0;
	// int ret_scanf;
	size_t file_size = 0;
	char str_status[15];
	char buffer[4096] ;
    memset(buffer,'\0',4096);
	char *p_buff = buffer;
	int buff_size = 4096;
	char *p_file_data = NULL;
	int header_len = 0;
	int bytes_wrote = 0;
	int total_bytes_wrote = 0;
	size_t data_remaining = 0;
	int rnrn_flag = 0;
	//Create request_str
    char message[BUFSIZ];
    memset(message,'\0',BUFSIZ);
    sprintf(message, "%s %s %s", "GETFILE GET", (*(*gfr)).req_path,"\r\n\r\n");

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
        if ((sockfd = socket(p->ai_family, p->ai_socktype,
                               p->ai_protocol)) == -1)
        {
            perror("client: socket");
            continue;
        }

        if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1)
        {
            close(sockfd);
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
    freeaddrinfo(servinfo); // all done with this structure
	send(sockfd, message, strlen(message), 0);

	//get header and data
	data_remaining = 1;
	while (data_remaining != 0)
	{
		buff_size = 4096;
		p_buff = buffer;
		bytes_read = 0;
		int init_bytes_read = 0;
		while (1)
		{
			//bytes_read = recv(sockfd, (void *)p_buff, buff_size,0);
			bytes_read = recv(sockfd, (void *)p_buff, 4096, 0);
			bytes_wrote = bytes_read;
			init_bytes_read += bytes_read;
			p_file_data = buffer;
			if (rnrn_flag == 1)
			{
				break;
			}
			else if (strstr(buffer, "\r\n\r\n") != NULL)
			{
				//found rnrn. Set flag
				rnrn_flag = 1;
				sscanf(buffer, "GETFILE %s %zu\r\n\r\n", str_status, &file_size);
				printf("str status = %s, filzesize = %zu\n", str_status, file_size);
				{
					sprintf(header, "GETFILE %s %zu\r\n\r\n", str_status, file_size);
					(*gfr)->status = GF_OK;
					(*gfr)->file_size = file_size;
					data_remaining = file_size;
                    
                        char sscheme[128];
                        char sstatus[128];
                        char placeholderint[128];
                        
                        // char g[128];
                        sscanf(buffer, "%s %s %s\r\n\r\n" , sscheme,sstatus,placeholderint);

                    // int message_length=strlen(sscheme)+strlen(sstatus)+strlen(placeholderint)+4+2;
					header_len = strlen(sscheme)+strlen(sstatus)+strlen(placeholderint)+4+2;
                    // header_len = strlen(header);
					bytes_wrote = init_bytes_read - header_len;
					p_file_data = buffer + header_len;
				}
				break;
			}
			buff_size -= bytes_read;
			p_buff += bytes_read;
		}
            (*gfr)->bytesreceived += bytes_wrote;
			total_bytes_read += bytes_read;
			total_bytes_wrote += bytes_wrote;
			data_remaining -= bytes_wrote;
			
			(*gfr)->writefunc((void *)p_file_data, bytes_wrote, (*gfr)->writearg);
		
	}
	// free(request_str);
	return 0;
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
    freeaddrinfo(servinfo); // all done with this structure
    char message[BUFSIZ];
    memset(message,'\0',BUFSIZ);
    sprintf(message, "%s %s %s", "GETFILE GET", (*(*gfr)).req_path,"\r\n\r\n");
    // strcpy(message,"GETFILE GET /courses/ud923/filecorpus/TestFile.txt\r\n\r\n");
    printf("Message is %s-----------\n",message);
    int charsRead=-1; 
    charsRead = send(socketFD, message, strlen(message), 0); // Send success back
    if (charsRead < 0)
      error("ERROR writing to socket");
    // Start Receiving the data
    // char completeMessage[2048];
    char *readBuffer;
    readBuffer=malloc(128);
    // memset(completeMessage, '\0', 2048);
    // memset(readBuffer, '\0', 128); // Clear the buffer
    int r=-1;
    // <scheme> <status> <length>\r\n\r\n<content>
    int total=0;
    // char *complete=readBuffer;
    memset(readBuffer, '\0', 128); // Clear the buffer
    while (strstr(readBuffer, "\r\n\r\n") == NULL) // As long as we haven't found the terminal...
    {
        r = recv(socketFD, (void*)readBuffer, 128, 0); // Get the next chunk
        
        total=total+r;
    }
    printf("Total Length till now:%d\n",total);
    // Check for getfile string format

    char sstatus[128];
    char placeholderint[128];
    
    // char g[128];
    sscanf(readBuffer, "GETFILE %s %s\r\n\r\n" , sstatus,placeholderint);
    size_t filelength=atoi(placeholderint);
    char header[128];
    sprintf(header, "GETFILE %s %ld\r\n\r\n", sstatus, filelength);
    printf("%s\n",header);
    int header_length=strlen(header);
    char *data;
    data=readBuffer+header_length;
    // data=malloc(filelength+message_length);
    // memset(data,'\0',filelength); //no null termination if complete data is filled correctly
    int bytesreceived=total-header_length;
    printf("Partial Data Received is %d\n",bytesreceived); 
    printf("Total File Length is %ld\n",filelength);   
    (*gfr)->writefunc((void *)data ,bytesreceived , (*gfr)->writearg);
    // readBuffer=NULL;
    int remaining_data=filelength-bytesreceived;
    while (remaining_data!=0) 
    {     
        r = recv(socketFD, (void*)readBuffer, 128 , 0);   
        if (r>0)
        {            
            bytesreceived=bytesreceived+r;
            remaining_data=remaining_data-r;
            // printf("Total Data Received:%d\n",bytesreceived);
            // printf("Total Data Remaining:%d\n",remaining_data);
            (*gfr)->writefunc((void *)readBuffer ,r , (*gfr)->writearg);
            memset(readBuffer, '\0', 128); // Clear the buffer
        }
        else
        {   
            printf("ERROR: EXITING LOOP; REmaining data is %d\n",remaining_data);
            break;
        }
        
    }    
    printf("\n%d %ld\n",bytesreceived,filelength);
    // free(readBuffer);
    // free(data);
    
    free(readBuffer);
    // free(data);
    
    close(socketFD);
    
    
    
    (*(*gfr)).status=GF_OK;//if file is sent from the server
        printf(" I AM HERE <<<<<<<<>>>>>>>>>>>\n");

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
    (*(*gfr)).writearg=writearg;
}

void gfc_set_writefunc(gfcrequest_t **gfr, void (*writefunc)(void*, size_t, void *)){
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

