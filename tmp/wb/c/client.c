#include<stdio.h>
#include<unistd.h>
#include<stdlib.h>
#include<limits.h>
#include<string.h>
#include<ctype.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<sys/stat.h>
#include<netdb.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<errno.h>
#include<dirent.h>

#define MAX_INPUT_SIZE 254
#define LOCAL_IP "127.0.0.1"
#define SERVER_PORT 2049
#define DATA_PORT 2048
#define FILEBUF_SIZE 1024
#define TYPE_I 0
#define TYPE_A 1
const char *MODE[] = {"BINARY","ASCII"};
int TYPE = 0;
char buffer[FILEBUF_SIZE];
char line_in[MAX_INPUT_SIZE+1];
char *server_ip, *local_ip;
char port_addr[24] = "";
int server_port, local_port;
int server_sock, client_sock = -1, data_sock;
int z = 0;
struct sockaddr_in server_addr, client_addr, data_addr;
struct hostent *server_host;


int replace(char *str, char *what, char *by, int max_length);


void send_msg(char *command, char *msg, int flag);


void help();
void user_login();

void command_list();//dir
void command_pwd();//pwd
void command_cd();//cd
void command_get(char *filename);//get
void command_put(char *filename);//put
void command_quit();//quit

void command_syst();//sys
void command_type();//type
int command_port();
int data_conn(char *ip, int port);

int main(int argv, char **argc)
{
    if(argv == 1)
    {
	server_ip = LOCAL_IP;
	server_port = SERVER_PORT;
    }
    else if(argv == 2)
    {
	server_ip = argc[1];
	server_port = SERVER_PORT;
    }
    else if(argv == 3)
    {
	server_ip = argc[1];
	server_port = atoi(argc[2]);
    }
    //connect to the ftp server
    server_host = gethostbyname(server_ip);
    if(server_host == (struct hostent *)NULL)
    {
	printf(">gethostbyname failed\n");
	exit(1);
    }
    //setup the port for the connection
    memset(&server_addr, 0, sizeof(server_addr));//将服务器端地址前sizeof(addr)字节置0，包括’\0’
    server_addr.sin_family = AF_INET;//AF_INET代表TCP/IP协议
    memcpy(&server_addr.sin_addr, server_host->h_addr, server_host->h_length);//服务器IP地址
    server_addr.sin_port = htons(server_port);//服务器端口号(使用网络字节的顺序)
    //get the socket
    server_sock = socket(PF_INET, SOCK_STREAM, 0);
    if(server_sock < 0)
    {
	printf(">error on socket()\n");
	exit(1);
    }
    //connect to the server
    if(connect(server_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
	printf(">error on connect()\n");
	close(server_sock);
	exit(1);
    }
    
    //connect to the ftp server successful
    printf(">Connected to %s:%d\n", server_ip, server_port);
    z = read(server_sock, buffer, sizeof(buffer));
    buffer[z-2] = 0;
    printf("%s\n", buffer);
    //login
    user_login();
    while(1)
    {
	//line_in[0] = 0;
	printf("wwb>");
	fgets(line_in, MAX_INPUT_SIZE, stdin);
	line_in[strlen(line_in)-1] = '\0';
	if(strncmp("quit", line_in, 4) == 0)
	{
	    command_quit();
	    break;
	}
	else if((strncmp("?", line_in, 1) == 0) || (strncmp("help", line_in, 4) == 0))
	{
	    help();
	}
	else if(strncmp("syst", line_in, 4) == 0)
	{
	    command_syst();
	}
	else if(strncmp("type", line_in, 4) == 0)
	{
	    command_type();
	}
	else if(strncmp("pwd", line_in, 3) == 0)
	{
	    command_pwd();
	}
	else if(strncmp("cd", line_in, 2) == 0)
	{
	    command_cd();
	}
	else if(strncmp("port", line_in, 4) == 0)
	{
	    command_port();
	}
	else if((strncmp("ls", line_in, 4) == 0) || (strncmp("dir", line_in, 3) == 0))
	{
	    command_list();
	}
	else if(strncmp("get", line_in, 3) == 0)
	{
	    command_get(&line_in[4]);
	}
	else if(strncmp("put", line_in, 3) == 0)
	{
	    command_put(&line_in[4]);
	}
	
    }
    close(server_sock);
    return 0;
}


//
int replace(char *str, char *what, char *by, int max_length)
{
    char *foo, *bar = str;
    int i = 0;
    int str_length, what_length, by_length;
    
    if (! str) return 0;
    if (! what) return 0;
    if (! by) return 0;
    
    what_length = strlen(what);
    by_length = strlen(by);
    str_length = strlen(str);
    
    foo = strstr(bar, what);

    while ( (foo) && ( (str_length + by_length - what_length) < (max_length - 1) ) ) 
    {
	bar = foo + strlen(by);
	memmove(bar, foo + strlen(what), strlen(foo + strlen(what)) + 1);
	memcpy(foo, by, strlen(by));
	i++;
	foo = strstr(bar, what);
	str_length = strlen(str);
    }
    return i;
}

//send msg to server
void send_msg(char *command, char *msg, int flag)
{
    char reply[MAX_INPUT_SIZE+1];
    if(flag == 0)
	sprintf(reply, "%s\r\n", command);//
    else
	sprintf(reply, "%s %s\r\n", command, msg);
    write(server_sock, reply, strlen(reply));
    printf("The command is %s", reply);
    return;
}

void user_login()
{
    printf(">User Name:");
    fgets(line_in, MAX_INPUT_SIZE, stdin);
    line_in[strlen(line_in)-1] = '\0';
    send_msg("USER", line_in, 1);
    
    z = read(server_sock, buffer, sizeof(buffer));
    buffer[z-2] = 0;
    printf("%s\n", buffer);
    if(strncmp("331", buffer, 3) == 0)
    {
	printf(">Password:");
	fgets(line_in, MAX_INPUT_SIZE, stdin);
	line_in[strlen(line_in)-1] = '\0';
	send_msg("PASS", line_in, 1);
	
	if(strncmp(line_in,"wwb",3)!=0)
	{
		printf("Password Error!\n");
		exit(0);
		
	}
	else
	{
		z = read(server_sock, buffer, sizeof(buffer));
        	buffer[z-2] = 0;
		printf("%s\n", buffer);
	}
	
		
	//}
	
	
    }
    line_in[0]=0;
    //command_syst();
    return;
}
void help()
{
    
    printf("?\tpwd\t");
    printf("cd\tport\tsyst\t");
    printf("get\tput\tquit\tdir\n");

}

void command_syst()
{
    send_msg("SYST", "", 0);
    z = read(server_sock, buffer, sizeof(buffer));
    buffer[z-2] = 0;
    printf("%s\n", buffer);
}

void command_quit()
{
    send_msg("QUIT", "", 0);
    z = read(server_sock, buffer, sizeof(buffer));
    buffer[z-2] = 0;
    printf("%s\n", buffer);
}

void command_type()
{
    char msg[2];
    msg[1] = 0;
    if(strncmp("ascii", &line_in[5], 5) == 0)
    {
	msg[0] = 'A';
	TYPE = TYPE_A;
    }
    else if(strncmp("binary", &line_in[5], 6) == 0)
    {
	msg[0] = 'I';
	TYPE = TYPE_I;
    }
    send_msg("TYPE", msg, 1);
    z = read(server_sock, buffer, sizeof(buffer));
    buffer[z-2] = 0;
    printf("%s\n", buffer);
}

void command_pwd()
{
    send_msg("PWD", "", 0);
    z = read(server_sock, buffer, sizeof(buffer));
    buffer[z-2] = 0;
    printf("%s\n", buffer);
}

void command_cd()
{
    send_msg("CWD", &line_in[3], 1);
    z = read(server_sock, buffer, sizeof(buffer));
    buffer[z-2] = 0;
    printf("%s\n", buffer);
}

int command_port()//active
{
    if(client_sock < 0)
    {
	struct sockaddr_in local_addr;
	socklen_t local_addr_len = sizeof local_addr;
	memset(&local_addr, 0, sizeof local_addr);
	if(getsockname(server_sock, (struct sockaddr *)&local_addr, &local_addr_len) != 0)
	{
	    printf("error when trying to get the local addr\n");
	    return -1;
	}
	local_ip = inet_ntoa(local_addr.sin_addr);
	local_port = local_addr.sin_port;
	printf("local addr  %s:%d\n", local_ip, local_port);
	char client_port[8] = "";
	sprintf(client_port, "%d.%d", (int)(local_port/256), (int)(1 + local_port - 256 * (int)(local_port/256)));
	sprintf(port_addr, "%s.%s", local_ip, client_port);
	replace(port_addr, ".", ",", 24);
	
	if(data_conn(local_ip, local_port + 1) == -1) return -1;
    }
    
    send_msg("PORT", port_addr, 1);
    z = read(server_sock, buffer, sizeof(buffer));
    buffer[z-2] = 0;
    printf("%s\n", buffer);
    
    //accept an request
    socklen_t data_addr_len = sizeof data_addr;
    memset(&data_addr, 0, sizeof data_addr);
    data_sock = accept(client_sock, (struct sockaddr *)&data_addr, &data_addr_len);
    if(data_sock < 0)
    {
	printf("data_sock accept failed!\n");
	return -1;
    }
    
    return 0;
}

int data_conn(char *ip, int port)
{
    client_sock = socket(PF_INET, SOCK_STREAM, 0);
    if(client_sock == -1)	//create socket failed
    {
	close(client_sock);
	printf("data socket() failed");
	return -1;
    }
    
    //configure server address,port
    memset(&client_addr, 0 ,sizeof(client_addr));
    socklen_t client_addr_len = sizeof client_addr;
    client_addr.sin_family = AF_INET;
    client_addr.sin_port = htons(port);
    client_addr.sin_addr.s_addr = inet_addr(ip);
    if(client_addr.sin_addr.s_addr == INADDR_NONE)
    {
	printf("INADDR_NONE");
	return -1;
    }
    
    //bind
    z = bind(client_sock, (struct sockaddr *)&client_addr, client_addr_len);
    if(z == -1)
    {
	close(client_sock);
	perror("data_sock bind failed\n");
	return -1;
    }
    
    //listen
    z = listen(client_sock, 1);
    if(z < 0)
    {
	close(client_sock);
	printf("data_sock listen failed!\n");
	return -1;
    }
    printf("data connect listening...\n");
    return 0;
}

void command_list()
{
    unsigned char databuf[PIPE_BUF];
    int bytes = 0;
    
    send_msg("TYPE", "A", 1);
    z = read(server_sock, buffer, sizeof(buffer));
    buffer[z-2] = 0;
    printf("%s\n", buffer);
    
    if(command_port() == -1) return;
    send_msg("LIST", "", 0);
    z = read(server_sock, buffer, sizeof(buffer));
    buffer[z-2] = 0;
    printf("%s\n", buffer);
    
    /* Read from the socket and write to stdout */
    while ((bytes = read(data_sock, databuf, sizeof(databuf)) > 0)) {
	//write(fileno(stdout), databuf, bytes);
	printf("%s", databuf);
	//printf("A\n");
    }
    
    /* Close the socket */
    close(data_sock);
    
    z = read(server_sock, buffer, sizeof(buffer));
    buffer[z-2] = 0;
    printf("%s\n", buffer);
}

void command_get(char *filename)
{
    FILE *outfile;
    short file_open=0;
    unsigned char databuf[FILEBUF_SIZE];
    int bytes = 0, bytesread = 0;
    
    if(command_port() == -1) return;
    send_msg("RETR", filename, 1);
    z = read(server_sock, buffer, sizeof(buffer));
    buffer[z-2] = 0;
    printf("%s\n", buffer);
    
    /* Read from the socket and write to the file */
    while ((bytes = read(data_sock, databuf, FILEBUF_SIZE)) > 0) {
	if (file_open == 0) {
	    /* Open the file the first time we actually read data */
	    if ((outfile = fopen(filename, "w")) == 0) {
		printf("fopen failed to open file");
		close(data_sock);
		return;
	    }
	    file_open = 1;
	}
	write(fileno(outfile), databuf, bytes);
	bytesread += bytes;
    }
    
    /* Close the file and socket */
    if (file_open != 0) fclose(outfile);
    close(data_sock);
    
    z = read(server_sock, buffer, sizeof(buffer));
    buffer[z-2] = 0;
    printf("%s\n", buffer);
    
    printf("%d bytes get.\n", bytesread);
}

void command_put(char *filename)
{
    FILE *infile;
    unsigned char databuf[FILEBUF_SIZE] = "";
    int bytes, bytessend=0;
    
    infile = fopen(filename,"r");
    if(infile == 0)
    {
	perror("fopen() failed");
	//close(data_sock);
	return;
    }
    
    if(command_port() == -1) return;
    send_msg("STOR", filename, 1);
    z = read(server_sock, buffer, sizeof(buffer));
    buffer[z-2] = 0;
    printf("%s\n", buffer);
    
    while((bytes = read(fileno(infile), databuf, FILEBUF_SIZE)) > 0)
    {
	if(TYPE == TYPE_A)
	{
	    replace((char *)databuf, "\r\n", "\n", FILEBUF_SIZE-1);
	    replace((char *)databuf, "\n", "\r\n", FILEBUF_SIZE-1);
	    write(data_sock, (const char *)databuf, strlen((const char *)databuf));
	}
	else if(TYPE == TYPE_I)
	{
	    write(data_sock, (const char *)databuf, bytes);
	}
	bytessend += bytes;
	memset(&databuf, 0, FILEBUF_SIZE);
    }
    memset(&databuf, 0, FILEBUF_SIZE);
    
    fclose(infile);
    close(data_sock);
    
    z = read(server_sock, buffer, sizeof(buffer));
    buffer[z-2] = 0;
    printf("%s\n", buffer);
    
    printf("%d bytes send\n", bytessend);
    return;
}












