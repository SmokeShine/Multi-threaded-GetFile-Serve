#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h> /* for memset */

int main()
{
   char addr_buf[64];
   struct addrinfo* feed_server = NULL;

   memset(addr_buf, 0, sizeof(addr_buf));

   getaddrinfo("localhost", NULL, NULL, &feed_server);
   struct addrinfo *res;
   for(res = feed_server; res != NULL; res = res->ai_next)
   {   
       if ( res->ai_family == AF_INET )
       {
          inet_ntop(AF_INET, &((struct sockaddr_in *)res->ai_addr)->sin_addr, addr_buf, sizeof(addr_buf));
       }
       else
       {
          inet_ntop(AF_INET6, &((struct sockaddr_in6 *)res->ai_addr)->sin6_addr, addr_buf, sizeof(addr_buf));
       }

       printf("hostname: %s\n", addr_buf); 
   } 

   return 0;
}