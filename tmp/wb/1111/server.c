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
#define SERVER_PORT 2049
#define DATA_PORT 2048
#define FILEBUF_SIZE 1024
#define TYPE_I 0
#define TYPE_A 1

void command_pwd(char *reply);
void command_cwd(char *dir);
void command_port(char *params, char *reply);
void command_list(char *reply);
void command_type(char *type, char *reply);
void command_retr(char *filename, char *reply);//下载 
void command_stor(char *filename, char *reply);//上传

int TYPE;
const char *MODE[] = {"BINARY","ASCII"};
char buffer[FILEBUF_SIZE];
int z;
int server_sock, client_sock, data_sock;
struct sockaddr_in server_addr, client_addr, data_addr;
struct hostent *data_host;
int server_adr_len;
socklen_t client_addr_len;
int i = 0;

void bail(const char *on_what)
{
    perror(on_what);
    exit(1);
}

//在长度范围内替换
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

int main(int argc, char **argv)
{
    
    //建立socket通信
    server_sock = socket(PF_INET, SOCK_STREAM, 0);
    //失败
    if(server_sock == -1)
    {
	bail("socket failed");
    }
    else printf("Socket created!\n");
    
    //配置地址端口号
    memset(&server_addr, 0 ,sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    if(server_addr.sin_addr.s_addr == INADDR_NONE)
    {
	bail("INADDR_NONE");
    }
    
    server_adr_len = sizeof server_addr;
    
    //绑定
    z = bind(server_sock, (struct sockaddr *)&server_addr, server_adr_len);
    if(z == -1)
    {
	bail("bind()");
    }
    else printf("Bind Ok!\n");
    
    //监听
    z = listen(server_sock, 5);
    if(z < 0)
    {
	printf("Server Listen Failed!");
	exit(1);
    }
    else printf("listening\n");
    
    //循环等待命令输入
    while(1)
    {
	TYPE = TYPE_I;
	client_addr_len = sizeof(client_addr);
	i = 0;
	printf("wait for accept...\n");
	//accept an request
	client_sock = accept(server_sock, (struct sockaddr *)&client_addr, &client_addr_len);
	if(client_sock < 0)
	{
	    
	    printf("Server Accept Failed!\n");
	    return 1;
	}
	else printf("Server Accept Succeed!New socket %d\n",client_sock);

	char reply[100] = "========================\n\nwelcome to my FTP server\n\n========================\n\n\r\n";
	write(client_sock, reply, strlen(reply));
	printf("%s",reply);

	while(1){
	    
	    
	    z = read(client_sock, buffer, sizeof(buffer));
	   buffer[z-2] = 0;
	    printf("command : '%s'\n",buffer);
	
	 
	    char command[3];
	    strncpy(command, buffer, 3);
	    command[3] = 0;
	    
	 
	    if(strncmp(command, "USE",3) == 0)//USER
	    {
		
		stpcpy(reply, "required Password\r\n");
		
		write(client_sock, reply, strlen(reply));
		printf("%s", reply);
	    }
	    else if(strncmp(command, "PAS",3) == 0)//PASS
	    {
		
		stpcpy(reply, "logged in\r\n");
		write(client_sock, reply, strlen(reply));
		printf("%s", reply);
		
		
	    }
	    else if(strncmp(command, "SYS",3) == 0)//SYST
	    {
		stpcpy(reply, "LINUX TYPE: L8\r\n");
		write(client_sock, reply, strlen(reply));
		printf("%s", reply);
	    }
	    else if(strncmp(command, "PWD",3) == 0)//PWD
	    {
		stpcpy(reply, "");
		command_pwd(reply);
		write(client_sock, reply, strlen(reply));
		printf("%s", reply);
	    }
	    else if(strncmp(command, "CWD",3) == 0)//CWD
	    {
		stpcpy(reply, "CWD command successful\r\n");
		command_cwd(&buffer[4]);
		write(client_sock, reply, strlen(reply));
		printf("%s", reply);
	    }
	    else if(strncmp(command, "POR",3) == 0)//PORT
	    {
		//stpcpy(reply, "200 PORT command successful\r\n");
		command_port(&buffer[5], reply);
		write(client_sock, reply, strlen(reply));
		printf("%s", reply);
	    }
	    else if(strncmp(command, "LIS",3) == 0)//LIST or DIR
	    {
		stpcpy(reply, "Opening ASCII mode data connection for file list\r\n");
		write(client_sock, reply, strlen(reply));
		printf("%s", reply);
		command_list(reply);
		write(client_sock, reply, strlen(reply));
		printf("%s", reply);
	    }
	    else if(strncmp(command, "TYP",3) == 0)//TYPE
	    {
		command_type(&buffer[5], reply);
		write(client_sock, reply, strlen(reply));
		printf("%s", reply);
	    }
	    else if(strncmp(command, "RET",3) == 0)//GET
	    {
		sprintf(reply, "Opening %s mode data connection for %s\r\n", MODE[TYPE], &buffer[5]);
		write(client_sock, reply, strlen(reply));
		printf("%s", reply);
		command_retr(&buffer[5], reply);
		stpcpy(reply, "Transfer complete\r\n");
		write(client_sock, reply, strlen(reply));
		printf("%s", reply);
	    }
	    else if(strncmp(command, "STO",3) == 0)//PUT
	    {
		sprintf(reply, "Opening %s mode data connection for %s\r\n", MODE[TYPE], &buffer[5]);
		write(client_sock, reply, strlen(reply));
		printf("%s", reply);
		command_stor(&buffer[5], reply);
		write(client_sock, reply, strlen(reply));
		printf("%s", reply);
	    }
	    else if(strncmp(command, "QUI",3) == 0)//QUIT
	    {
		stpcpy(reply, "Goodbye.\r\n");
		write(client_sock, reply, strlen(reply));
		printf("%s", reply);
		break;
	    }
	}
	
	close(client_sock);
	printf("\n");
    }
    close(server_sock);
    return 0;
}




void command_pwd(char *reply)
{
    char buf[255];
    getcwd(buf,sizeof(buf)-1);
    strcat(reply,buf);
    strcat(reply,"\r\n");
    return;
}

void command_cwd(char *dir)
{
    
    chdir(dir);
}

void command_port(char *params, char *reply)
{
    unsigned long a0, a1, a2, a3, p0, p1, addr;
    sscanf(params, "%lu,%lu,%lu,%lu,%lu,%lu", &a0, &a1, &a2, &a3, &p0, &p1);
    addr = htonl((a0 << 24) + (a1 << 16) + (a2 << 8) + a3);
    if(addr != client_addr.sin_addr.s_addr)
    {
	stpcpy(reply, "The given address is not yours\r\n");
	return;
    }
    
    //数据连接端口号
    memset(&data_addr, 0, sizeof(data_addr));
    data_addr.sin_family = AF_INET;
    data_addr.sin_addr.s_addr = addr;
    data_addr.sin_port = htons((p0 << 8) + p1);
    
    //建立数据连接
    data_sock = socket(PF_INET, SOCK_STREAM, 0);
    if(data_sock == -1)
    {
	bail("data_sock failed");
	stpcpy(reply, "Cannot get the data socket\r\n");
	return;
    }
    
    //和客户端建立数据连接
    if(connect(data_sock, (struct sockaddr *)&data_addr, sizeof(data_addr)) < 0)
    {
	bail("connect!");
	stpcpy(reply, "Cannot connect to the data socket\r\n");
	return;
    }
    
    stpcpy(reply, "PORT command successful\r\n");
}


void command_list(char *reply)
{
    FILE *fcmd;
    char databuf[PIPE_BUF];
    int n;
    
    fcmd = popen("ls -l","r");
    if(fcmd == 0)
    {
	bail("popen failed");
	stpcpy(reply, "Transfer error\r\n");
	close(data_sock);
	return;
    }
    memset(databuf, 0, PIPE_BUF);
    while((n = read(fileno(fcmd), databuf, PIPE_BUF)) > 0)
    {
	replace(databuf, "\n", "\r\n", PIPE_BUF-1);
	printf("---->%s", databuf);
	write(data_sock, databuf, strlen(databuf));
	memset(databuf, 0, PIPE_BUF);
    }
    memset(databuf, 0,PIPE_BUF);
    if(pclose(fcmd) != 0)
    {
	bail("Non-zero return value from \"ls -l\"");
    }
    close(data_sock);
    stpcpy(reply, "Transfer complete\r\n");
    //memset(databuf, 0, PIPE_BUF - 1);
    return;
}
//数据传输类型
void command_type(char *type, char *reply)
{
    switch(type[0])
    {
	case 'A':
	    TYPE = TYPE_A;
	    stpcpy(reply, "Type set to A\r\n");
	    break;
	case 'I':
	    TYPE = TYPE_I;
	    stpcpy(reply, "Type set to I\r\n");
	    break;
	default:
	    TYPE = TYPE_I;
	    stpcpy(reply, "Type set to I\r\n");
	    break;
    }
    return;
}
//下载 
void command_retr(char *filename, char *reply)
{
    FILE *infile;
    unsigned char databuf[FILEBUF_SIZE] = "";
    int bytes;
    
    infile = fopen(filename,"r");
    if(infile == 0)
    {
	perror("fopen() failed");
	close(data_sock);
	return;
    }
    
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
	memset(&databuf, 0, FILEBUF_SIZE);
    }
    memset(&databuf, 0, FILEBUF_SIZE);
    
    fclose(infile);
    close(data_sock);
    stpcpy(reply, "Transfer complete\r\n");
    return;
}
//上传
void command_stor(char *filename, char *reply)
{
    FILE *outfile;
    unsigned char databuf[FILEBUF_SIZE];
    struct stat sbuf;
    int bytes = 0;
    
    if(stat(filename, &sbuf) == -1)
    {
	printf("File not exist,try to create a new one.\n");
    }

    outfile = fopen(filename, "w");
    if(outfile == 0)
    {
	bail("fopen() failed");
	stpcpy(reply, "Cannot create the file\r\n");
	close(data_sock);
	return;
    }
    
    while((bytes = read(data_sock, databuf, FILEBUF_SIZE)) > 0)
    {
	write(fileno(outfile), databuf, bytes);
    }
    
    fclose(outfile);
    close(data_sock);
    stpcpy(reply, "Transfer complete\r\n");
    return;
}

