
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

	/*
	 * Performs the transfer as described in the options.  Returns a value of 0
	 * if the communication is successful, including the case where the server
	 * returns a response with a FILE_NOT_FOUND or ERROR response.  If the
	 * communication is not successful (e.g. the connection is closed before
	 * transfer is complete or an invalid header is returned), then a negative
	 * integer will be returned.
	 */
	int sockfd = 0; //socket file descriptor
	// struct sockaddr_in serv_addr;
	const char *line_ending = "\r\n\r\n";
	const char *line_beg = "GETFILE GET";
	char header[1000] = "";
	// char *p_header = header;
	int bytes_read = 0;
	int total_bytes_read = 0;
	// int header_size = 200;
	int ret_scanf;
	size_t file_size = 0;
	char str_status[15];
	char buffer[4096] = "";
	char *p_buff = buffer;
	int buff_size = 4096;
	char *p_file_data = NULL;
	int header_len = 0;
	int bytes_wrote = 0;
	int total_bytes_wrote = 0;
	size_t data_remaining = 0;
	int rnrn_flag = 0;
	// int malform_flag = 0;
	//Create request_str
	char *request_str = (char *)malloc(strlen(line_beg) + strlen((*gfr)->req_path) + strlen(line_ending) + 3);
	sprintf(request_str, "%s %s%s", line_beg, (*gfr)->req_path, line_ending);
	printf("%ld\n",strlen(request_str));
	//test prints
	printf("This is the server: %s\n", (*gfr)->server);
	printf("This is the port: %d\n", (*gfr)->port);
	printf("This is the path: %s\n", (*gfr)->req_path);
	printf("This is the request str: %s\n", request_str);

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
//    printf("******%s ******\n\n %s  \n\n",PORT,(*(*gfr)).server);

    //  char s[INET6_ADDRSTRLEN];

    // inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr),
    //         s, sizeof s);
    // printf("%s---",s);
    // printf("%d---",p->ai_family);

    freeaddrinfo(servinfo); // all done with this structure
        
	if (sockfd < 0)
	{
		perror("hello\n");
		return -1;
	}

	printf("CLIENT: This is the request str sent: %s\n", request_str);
	send(sockfd, request_str, strlen(request_str), 0);

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
			if (bytes_read < 0)
			{
				//error reading socket
				perror("CLIENT: ERROR READING SOCKET\n");
				return -1;
			}
			if (rnrn_flag == 1)
			{
				break;
			}
			else if (bytes_read == 0)
			{
				//never found \r\n\r\n flag. Malformed request
				(*gfr)->status = GF_FILE_NOT_FOUND;
				(*gfr)->file_size = 0;
			}
			else if (strstr(buffer, line_ending) != NULL)
			{
				//found rnrn. Set flag
				rnrn_flag = 1;
				ret_scanf = sscanf(buffer, "GETFILE %s %zu\r\n\r\n", str_status, &file_size);
				printf("str status = %s, filzesize = %zu\n", str_status, file_size);
				if (ret_scanf == EOF || ret_scanf < 2)
				{
					ret_scanf = sscanf(buffer, "GETFILE %s \r\n\r\n", str_status);
					perror("PATH WAS PARSED but fpath not found\n");
					(*gfr)->status = GF_ERROR;
					break;
				}
				else
				{
					sprintf(header, "GETFILE %s %zu\r\n\r\n", str_status, file_size);
					printf("this is the header: %s\n", header);
					(*gfr)->status = GF_OK;
					(*gfr)->file_size = file_size;
					data_remaining = file_size;
					header_len = strlen(header);
					bytes_wrote = init_bytes_read - header_len;
					p_file_data = buffer + header_len;
				}
				break;
			}
			else
			{
				puts("No rnrn found. Loop again");
			}
			buff_size -= bytes_read;
			p_buff += bytes_read;
		}

		//Finished reading
		if (bytes_read == 0)
		{
			break;
		}
		else
		{
			//first loop will contain header
			//parse to find information

			if ((*gfr)->writefunc == NULL || (*gfr)->status != GF_OK)
			{
				puts("CLIENT: WRITEFUNC not set or Filestatus != OK");
				break;
			}
			else
				(*gfr)->bytesreceived += bytes_wrote;
			total_bytes_read += bytes_read;
			total_bytes_wrote += bytes_wrote;
			data_remaining -= bytes_wrote;
			
			(*gfr)->writefunc((void *)p_file_data, bytes_wrote, (*gfr)->writearg);
		}
	}

	puts("COMPLETE CLIENT\n");
	free(request_str);
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
//    printf("******%s ******\n\n %s  \n\n",PORT,(*(*gfr)).server);

    //  char s[INET6_ADDRSTRLEN];

    // inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr),
    //         s, sizeof s);
    // printf("%s---",s);
    // printf("%d---",p->ai_family);

    freeaddrinfo(servinfo); // all done with this structure
        
    // Create the request string to be send to the server
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
    // may or may not contain header
    // <scheme> <status> <length>\r\n\r\n<content>
    // may be writing part is handler work? will never know 
    // cannot use strlen this time - be careful 
    // fixed size request
    // convert to ascii? lol

    char completeMessage[2048];
    char *readBuffer;
    readBuffer=malloc(128);
    memset(completeMessage, '\0', 2048);
    memset(readBuffer, '\0', 128); // Clear the buffer

    // you are receiving bytes because of void - it is still a string without \0
    // Do byte level comparision? 
    // check beej guide
    //Brute Force - Check for end of header flag. Block transfer data till then. 
    // After finding the header flag, check back for format
    int r=-1;
    // <scheme> <status> <length>\r\n\r\n<content>
    int total=0;
    while (strstr(completeMessage, "\r\n\r\n") == NULL) // As long as we haven't found the terminal...
    {
        memset(readBuffer, '\0', 128); // Clear the buffer
        r = recv(socketFD, (void*)readBuffer, 127, 0); // Get the next chunk
        strcat(completeMessage, readBuffer); // Add that chunk to what we have so far
        total=total+r;
        // printf("PARENT: Message received from Server: \"%s\", total: \"%s\"\n", readBuffer, completeMessage);
        // if (r == -1) { printf("r == -1\n"); break; } // Check for errors
        // if (r == 0) { break; }
        // printf("Bytes Received From Handler - %d\n",r);
        // printf("%s\n", readBuffer);       
    }
    printf("*****###################\n");
    // Check for getfile string format
    //buffer may still have some data
    char sscheme[128];
    char sstatus[128];
    char placeholderint[128];
    int filelength=-1;
    // char g[128];
    sscanf(completeMessage, "%s %s %s\r\n\r\n" , sscheme,sstatus,placeholderint);
    filelength=atoi(placeholderint);
    int message_length=strlen(sscheme)+strlen(sstatus)+strlen(placeholderint)+4+2;
    printf("Total Received from header is %d\n",total);
    printf("Header is %d\n",message_length);
    char *data;
    data=malloc(filelength+message_length);
    // memset(data,'\0',filelength); //no null termination if complete data is filled correctly
    int bytesreceived=total-message_length;
    printf("Partial Data Received is %d\n",bytesreceived);

    // memcpy(data, completeMessage+strlen(sscheme)+strlen(sstatus)+strlen(placeholderint)+6, bytesreceived);

    data=completeMessage+message_length; // starting address of file data
    // printf("Here are the contents of the entire buffer:\n");
    // printf(" # CHAR INT\n");
    // for (int currChar = 0; currChar < total; currChar++) // Display every character in both dec and ASCII
    // {
    //     printf("%3d `%c\' %3d\n", currChar, completeMessage[currChar], completeMessage[currChar]);
    // }
    // FILE *file;
    // printf(" I AM HERE\n");
    // file = openFile("courses/ud923/filecorpus/paraglider.jpg_000000"); 
    
    
    // writecb(data, bytesreceived, file);
    // printf("====%s===%d===\n",data,bytesreceived);
    (*gfr)->writefunc((void*)data ,bytesreceived , (*gfr)->writearg); //not working. 
    // data=data+bytesreceived;
    // printf("%ld--%s\n",strlen(sscheme),sscheme);
    // printf("%ld--%s\n",strlen(sstatus),sstatus);
    // printf("%ld--%s\n",strlen(placeholderint),placeholderint);
    // printf("%ld--\n",strlen(completeMessage));
    // printf("%ld header+%d data\n",strlen(sscheme)+strlen(sstatus)+strlen(placeholderint)+4+2,bytesreceived);
    // read till you dont receive any data anymore
    // memset(readBuffer, '\0', 128); // Clear the buffer
    // bytesreceived=0;
    while (bytesreceived<filelength) 
    {     
        r = recv(socketFD, readBuffer, 128 , 0);   
        if (r>0)
        {
            
            bytesreceived=bytesreceived+r;
            // data=data+r;
    // Disable writing till you cant read the header
    // printf("*************************************\n");
            printf(" Bytes Received=%d\n",bytesreceived);
            (*gfr)->writefunc((void *)readBuffer ,r , (*gfr)->writearg);
            // printf("*************************************\n");
            
            memset(readBuffer, '\0', 128); // Clear the buffer
        }
        else
        {   

            printf("ERROR: EXITING LOOP\n");
            break;
        }
        
    }    
    
    // free(readBuffer);
    // free(data);

        // memcopy, then check for header end - how to loop?
        // there is length field - but in bytes. how to check if response is getfile get again?
    //second parameter is size. third parameter is a function 
    //leap of faith. match with writecb - nonsense - what happens to the bytes read?
    
    // free(readBuffer);
    // free(data);
    
    // close(socketFD);
    
    
    printf("\n%d %d\n",bytesreceived,filelength);
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
    //memset to \0
    // memset((*(*gfr)).writearg,'\0',sizeof(writearg)+1);
    // strcpy may not work
    (*(*gfr)).writearg=writearg;
}

void gfc_set_writefunc(gfcrequest_t **gfr, void (*writefunc)(void*, size_t, void *)){
    // function pointer is an argument - looks similar to writecb
    printf("I AM HERE *****************\n");
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

