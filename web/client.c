/*Client Code*/
/* The following main code from https://github.com/ankushagarwal/nweb, but they are modified slightly
*/
#include <stdio.h>
#include <stdlib.h> 
#include <unistd.h> 
#include <string.h> 
#include <sys/types.h> 
#include <sys/socket.h> 
#include <netinet/in.h> 
#include <arpa/inet.h>

/* IP address and port number */

#define PORT	8181	
// Port number as an integer - web server default is 80 #define IP_ADDRESS "192.168.0.8"	
//IP Address as a string
/* Request a html file base on HTTP */
char *httprequestMsg = "GET /helloworld.html HTTP/1.0 \r\n\r\n" ;
#define BUFSIZE 8196 void pexit(char * msg)
{
    perror(msg); exit(1);
}
void main()
{
    int i,sockfd;
    char buffer[BUFSIZE];
    static struct sockaddr_in serv_addr;

    printf("client trying to connect to %s and port %d\n",IP_ADDRESS,PORT); if((sockfd = socket(AF_INET, SOCK_STREAM,0)) <0) //create a client socket
    pexit("socket() error");

    serv_addr.sin_family = AF_INET;	//Set the socket with IPv4 serv_addr.sin_addr.s_addr = inet_addr(IP_ADDRESS);// set ip address serv_addr.sin_port = htons(PORT); // set ip port number

    /* Connect the socket offered by the web server */
    if(connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) <0) pexit("connect() error");

    /* Now the sockfd can be used to communicate to the server the GET request */ printf("Send bytes=%d %s\n",strlen(httprequestMsg), httprequestMsg); write(sockfd, httprequestMsg, strlen(httprequestMsg));

    /* This displays the raw HTML file (if index.html) as received by the browser */ while( (i=read(sockfd,buffer,BUFSIZE)) > 0)
    write(1,buffer,i);
    /*close the socket*/
    close(sockfd);
}

