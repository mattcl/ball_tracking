#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 
#include <strings.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>

void error(char *msg)
{
    perror(msg);
    exit;
}

int connect_client(int* sockfd, char * buffer, const char * ip_addr)
{
    struct sockaddr_in serv_addr;
    struct hostent *server;
    int x;
    *sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) 
        error("ERROR opening socket");
	server = gethostbyname(ip_addr);
	//server = gethostbyname("192.168.2.4");
	//	server = gethostbyname("localhost");
    if (server == NULL) {
        fprintf(stderr,"ERROR, no such host\n");
        exit(0);
    }
    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr, 
		  (char *)&serv_addr.sin_addr.s_addr,
		  server->h_length);
    serv_addr.sin_port = htons(8080);
    if (connect(*sockfd,&serv_addr,sizeof(serv_addr)) < 0) 
        error("ERROR connecting");
    return 0;
}

int read_client(int newsockfd, char * buffer, int length_b, int length)
{
	int n;
	bzero(buffer,length_b);
	n = read(newsockfd,buffer,length);
	return n;
}

int write_client(int sockfd, char * buffer, int length)
{
	int n;
	n = write(sockfd,buffer,length);
	return n;
}

/*
 int main()
 {
 int sockfd;
 int num = 0;
 char buffer[256];
 connect_client(&sockfd, buffer);
 while(1){
 num++;
 sprintf(buffer,"%d\n",num);
 write_client(sockfd,buffer);
 }
 }
 */
