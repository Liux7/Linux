/*Server Code*/
/* webserver.c*/
/*The following main code from https://github.com/ankushagarwal/nweb*, but they are modified slightly*/
#include <stdio.h> 
#include <stdlib.h> 
#include <unistd.h> 
#include <semaphore.h>
#include <errno.h> 
#include <string.h> 
#include <fcntl.h> 
#include <time.h>
#include <signal.h> 
#include <sys/types.h> 
#include <sys/socket.h> 
#include <netinet/in.h> 
#include <arpa/inet.h> 
#include <sys/time.h>
#include <sys/mman.h>

#define VERSION 23
#define BUFSIZE 8096
#define ERROR	42
#define LOG	44
#define FORBIDDEN 403
#define NOTFOUND	404
#ifndef SIGCLD
    #define SIGCLD SIGCHLD 
#endif

#define SEM_NAME "sem_example" 
#define SHM_NAME "mmap_example"

void *tmpargv;
// void * totaltime, *linkcnt;

struct { char *ext;
char *filetype;
} extensions [] = {
{"gif", "image/gif" },
{"jpg", "image/jpg" },
{"jpeg","image/jpeg"},
{"png", "image/png" },
{"ico", "image/ico" },
{"zip", "image/zip" },
{"gz", "image/gz" },
{"tar", "image/tar" },
{"htm", "text/html" },
{"html","text/html" },
{0,0} };




void logger(int type, char *s1, char *s2, int socket_fd)
{

    int fd ;
    char logbuffer[BUFSIZE*2];

	
    switch (type) {
        case ERROR: (void)sprintf(logbuffer,"ERROR: %s:%s Errno=%d exiting pid=%d",s1, s2, errno,getpid()); break;
        case FORBIDDEN:
        (void)write(socket_fd, "HTTP/1.1 403 Forbidden\nContent-Length: 185\nConnection: close\nContent-Type:	text/html\n\n<html><head>\n<title>403 Forbidden</title>\n</head><body>\n<h1>Forbidden</h1>\nThe requested URL, file type or operation is not allowed on this simple static file webserver.\n</body></html>\n",271);
        (void)sprintf(logbuffer,"FORBIDDEN: %s:%s",s1, s2); break;
        case NOTFOUND:
        (void)write(socket_fd, "HTTP/1.1 404 Not Found\nContent-Length: 136\nConnection: close\nContent-Type:	text/html\n\n<html><head>\n<title>404	Not Found</title>\n</head><body>\n<h1>Not Found</h1>\nThe requested URL was not found on this server.\n</body></html>\n",224);
        (void)sprintf(logbuffer,"NOT FOUND: %s:%s",s1, s2); break;
        case LOG: 
			(void)sprintf(logbuffer,"INFO: %s:%s:%d",s1, s2,socket_fd); 
			break;
    }

	/*=================================*/
	char timebuff[60];
	time_t timer;
	struct tm *Now;
	time(&timer);
	Now = localtime(&timer);

	int fp = open("webserver.log",O_WRONLY | O_APPEND);
	(void)sprintf(timebuff, "[%s", asctime(Now));

	(void)write(fp, timebuff, strlen(timebuff));
	(void)close(fp);
	/*===================================*/

    /* No checks here, nothing can be done with a failure anyway */
    if((fd = open("webserver.log", O_CREAT| O_WRONLY | O_APPEND,0644)) >= 0) { 
		(void)write(fd,logbuffer,strlen(logbuffer));
	    (void)write(fd,"\n",1); 
		(void)close(fd);
    }
    if(type == ERROR || type == NOTFOUND || type == FORBIDDEN) exit(3);
}

void* totalsR, *totalsW, *totallog, *totalrhtml;
double socketRead = 0.0, socketWrite = 0.0, logtime = 0.0, readhtml = 0.0;

/* this is a child web server process, so we can exit on errors */ 
void web(int fd, int hit)
{
    struct timeval t0, t1, t2, t3;

    
    double tmptime = 0.0;
    int j, file_fd, buflen; long i, ret, len; char * fstr;
    static char buffer[BUFSIZE+1]; /* static so zero filled */

    /*===========================read socket=================================*/
    gettimeofday(&t0, NULL);
    ret =read(fd,buffer,BUFSIZE);	/* read Web request in one go */ 
    gettimeofday(&t1, NULL);
    tmptime = (t1.tv_sec - t0.tv_sec)*1e6 + (t1.tv_usec - t0.tv_usec); 
    socketRead += tmptime;

    /*==============================log=========================================*/
    gettimeofday(&t0, NULL);

    if(ret == 0 || ret == -1) { /* read failure stop now */
        logger(FORBIDDEN,"failed to read browser request","",fd);
    }
    if(ret > 0 && ret < BUFSIZE) /* return code is valid chars */ 
        buffer[ret]=0;	/* terminate the buffer */
    else 
        buffer[0]=0;


        
    for(i=0;i<ret;i++) /* remove CF and LF characters */ 
        if(buffer[i] == '\r' || buffer[i] == '\n')
            buffer[i]='*'; 

    logger(LOG,"request",buffer,hit);
    if( strncmp(buffer,"GET ",4) && strncmp(buffer,"get ",4) ) 
    { 
        logger(FORBIDDEN,"Only simple GET operation supported",buffer,fd);
    }
    for(i=4;i<BUFSIZE;i++) { /* null terminate after the second space to ignore extra stuff */ 
        if(buffer[i] == ' ') 
        { /* string is "GET URL " +lots of other stuff */
            buffer[i] = 0; break;
        }
    }
    for(j=0;j<i-1;j++)	/* check for illegal parent directory use .. */ 
    if(buffer[j] == '.' && buffer[j+1] == '.') 
    {
        logger(FORBIDDEN,"Parent directory (..) path names not supported",buffer,fd);
 
    }
    if( !strncmp(&buffer[0],"GET /\0",6) || !strncmp(&buffer[0],"get /\0",6) ) 
        (void)strcpy(buffer,"GET /index.html");
 
    gettimeofday(&t1, NULL);
    tmptime = (t1.tv_sec - t0.tv_sec)*1e6 + (t1.tv_usec - t0.tv_usec); 
    logtime += tmptime;
    

    /* work out the file type and check we support it */ 
    buflen=strlen(buffer);
    fstr = (char *)0; 
    for(i=0;extensions[i].ext != 0;i++) 
    {
        len = strlen(extensions[i].ext);
        if( !strncmp(&buffer[buflen-len], extensions[i].ext, len)) 
        { 
            fstr =extensions[i].filetype;
            break;
        }
    }
    if(fstr == 0) logger(FORBIDDEN,"file extension type not supported",buffer,fd);

    if(( file_fd = open(&buffer[5],O_RDONLY)) == -1) 
    { /* open the file for reading */ 
        logger(NOTFOUND, "failed to open file",&buffer[5],fd);
    }

/*==============================log=========================================*/
    gettimeofday(&t0, NULL);
    logger(LOG,"SEND",&buffer[5],hit);
    
    

    len = (long)lseek(file_fd, (off_t)0, SEEK_END); /* lseek to the file end to find the length */ 
    (void)lseek(file_fd, (off_t)0, SEEK_SET); /* lseek back to the file start ready for reading */
    (void)sprintf(buffer,"HTTP/1.1 200 OK\nServer: nweb/%d.0\nContent-Length: %ld\nConnection: \
    close\nContent-Type: %s\n\n", VERSION, len, fstr); /* Header + a blank line */ 

   
    logger(LOG,"Header",buffer,hit);
    gettimeofday(&t1, NULL);
    tmptime = (t1.tv_sec - t0.tv_sec)*1e6 + (t1.tv_usec - t0.tv_usec); 
    logtime += tmptime;
    
    /*===========================write socket=================================*/
    gettimeofday(&t0, NULL);
    (void)write(fd,buffer,strlen(buffer));
    gettimeofday(&t1, NULL);
    tmptime = (t1.tv_sec - t0.tv_sec)*1e6 + (t1.tv_usec - t0.tv_usec); 
    socketWrite += tmptime;


/* send file in 8KB block - last block may be smaller */ 
    gettimeofday(&t0, NULL);
    while ((ret = read(file_fd, buffer, BUFSIZE)) > 0) 
    {
        (void)write(fd,buffer,ret);
    }
    gettimeofday(&t1, NULL);
    tmptime = (t1.tv_sec - t0.tv_sec)*1e6 + (t1.tv_usec - t0.tv_usec);
    readhtml += tmptime;
    sleep(1); /* allow socket to drain before signalling the socket is closed */ 
    close(fd);
   // exit(1);
   
   

}

int main(int argc, char **argv)
{

    int i, pid, port, listenfd, socketfd, hit; socklen_t length;
    static struct sockaddr_in cli_addr; /* static = initialised to zeros */ static struct sockaddr_in serv_addr; /* static = initialised to zeros */
/*deal input format begin*/
    if( argc < 3 || argc > 3 || !strcmp(argv[1], "-?") ) {
        (void)printf("hint: nweb Port-Number Top-Directory\t\tversion %d\n\n" "\tnweb is a small and very safe mini web server\n"
        "\tnweb only servers out file/web pages with extensions named below\n" "\t and only from the named directory or its sub-directories.\n"
        "\tThere is no fancy features = safe and secure.\n\n" "\tExample:webserver 8181 /home/nwebdir &\n\n" "\tOnly Supports:", VERSION);
        for(i=0;extensions[i].ext != 0;i++) 
            (void)printf(" %s",extensions[i].ext);

        (void)printf("\n\tNot Supported: URLs including \"..\", Java, Javascript, CGI\n" "\tNot Supported: directories / /etc /bin /lib /tmp /usr /dev /sbin \n"
        "\tNo warranty given or implied\n\tNigel Griffiths nag@uk.ibm.com\n" ); 
        exit(0);
    }
    if( !strncmp(argv[2],"/"	,2 ) || !strncmp(argv[2],"/etc", 5 ) ||
    !strncmp(argv[2],"/bin",5 ) || !strncmp(argv[2],"/lib", 5 ) ||
    !strncmp(argv[2],"/tmp",5 ) || !strncmp(argv[2],"/usr", 5 ) ||
    !strncmp(argv[2],"/dev",5 ) || !strncmp(argv[2],"/sbin",6) ){ (void)printf("ERROR: Bad top directory %s, see nweb -?\n",argv[2]); 
    exit(3);
    }
    if(chdir(argv[2]) == -1){
    (void)printf("ERROR: Can't Change to directory %s\n",argv[2]); exit(4);
    }
/*deal input format end*/

	
    /* Become deamon + unstopable and no zombies children (= no wait()) */ 
    
    if(fork() != 0)
        return 0; /* parent returns OK to shell */ 
    (void)signal(SIGCLD, SIG_IGN); /* ignore child death */ 
    (void)signal(SIGHUP, SIG_IGN); /* ignore terminal hangups */ 
    for(i=0;i<32;i++)
        (void)close(i);	/* close open files */
    (void)setpgrp();	/* break away from process group */ 
    logger(LOG,"nweb starting",argv[1],getpid()); /* setup the network socket */
    
    if((listenfd = socket(AF_INET, SOCK_STREAM,0)) <0) 
        logger(ERROR, "system call","socket",0);
    port = atoi(argv[1]); 
    if(port < 0 || port >60000)
        logger(ERROR,"Invalid port number (try 1->60000)",argv[1],0); 
    
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    
    serv_addr.sin_port = htons(port);
    if(bind(listenfd, (struct sockaddr *)&serv_addr,sizeof(serv_addr)) <0) 
        logger(ERROR,"system call","bind",0);
        
    if( listen(listenfd,64) <0) 
        logger(ERROR,"system call","listen",0);
	
    sem_t* psem;
    //创建信号量,初始信号量为 1
    
    if((psem=sem_open(SEM_NAME, O_CREAT,0666, 1))==SEM_FAILED)
    {
        perror("create semaphore error"); 
        exit(1);
    }
    int shm_fd;
    // , shm_fd1, shm_fd2, shm_fd3, shm_fd4, shm_fd5;
    //创建共享内存对象
    if((shm_fd=shm_open(SHM_NAME,O_RDWR| O_CREAT,0666)) < 0){ 
        perror("create shared memory object error");
        exit(1);
    }
    // if((shm_fd1=shm_open(SHM_NAME,O_RDWR| O_CREAT,0666)) < 0){ 
    //     perror("create shared memory object error");
    //     exit(1);
    // }
    // if((shm_fd2=shm_open(SHM_NAME,O_RDWR| O_CREAT,0666)) < 0){ 
    //     perror("create shared memory object error");
    //     exit(1);
    // }
    // if((shm_fd3=shm_open(SHM_NAME,O_RDWR| O_CREAT,0666)) < 0){ 
    //     perror("create shared memory object error");
    //     exit(1);
    // }
    // if((shm_fd4=shm_open(SHM_NAME,O_RDWR| O_CREAT,0666)) < 0){ 
    //     perror("create shared memory object error");
    //     exit(1);
    // }
    // if((shm_fd5=shm_open(SHM_NAME,O_RDWR| O_CREAT,0666)) < 0){ 
    //     perror("create shared memory object error");
    //     exit(1);
    // }
    /* 配 置 共 享 内 存 段 大 小 */ 
    ftruncate(shm_fd, sizeof(double)*6);
    // ftruncate(shm_fd1, sizeof(double));
    // ftruncate(shm_fd2, sizeof(double));
    // ftruncate(shm_fd3, sizeof(double));
    // ftruncate(shm_fd4, sizeof(double));
    // ftruncate(shm_fd5, sizeof(int));
    //将共享内存对象映射到进程
    tmpargv = mmap(NULL, sizeof(double)*6, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    // totalsR = mmap(NULL, sizeof(double), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd1, 0);
    // totalsW = mmap(NULL, sizeof(double), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd2, 0);
    // totallog = mmap(NULL, sizeof(double), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd3, 0);
    // totalrhtml = mmap(NULL, sizeof(double), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd4, 0);
    // linkcnt = mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd4, 0);

    if(tmpargv == MAP_FAILED)
    {
        perror("create mmap error"); 
        exit(1);
    }
    
    //为此内存区域赋值
    // * (double *) totaltime= 0.0;
    for(int i = 0; i < 6; i++)
    {
        *(double *)(tmpargv+i*sizeof(double)) = 0.0;
    }
    struct timeval t1, t2;
		
    for(hit=1; ;hit++) {
	    if((socketfd = accept(listenfd, (struct sockaddr *)&cli_addr, &length)) < 0)
			logger(ERROR,"system call","accept",0);
        if((pid = fork()) < 0) {
  	        logger(ERROR,"system call","fork",0);
  	    }
	    else
	    {
		
		  if(pid == 0) {   /* child */
    	    (void)close(listenfd);

            gettimeofday(&t1, NULL);
    	    web(socketfd,hit); /* never returns */
            gettimeofday(&t2, NULL);

            double ptime =  (t2.tv_sec - t1.tv_sec)*1000 + (t2.tv_usec - t1.tv_usec)/1000;
            // double ptime =  (t2.tv_sec - t1.tv_sec)*1000; 
			sem_wait(psem);
            *(double *)tmpargv += ptime;
            (*(double *)(tmpargv+1*sizeof(double))) += (socketRead/1000);
            (*(double *)(tmpargv+2*sizeof(double))) += (socketWrite/1000);
            (*(double *)(tmpargv+3*sizeof(double))) += (logtime/1000);
            (*(double *)(tmpargv+4*sizeof(double))) += (readhtml/1000);
            (*(double*)(tmpargv+5*sizeof(double)))++;
            int cnt = (int)(*(double*)(tmpargv+5*sizeof(double)));
            /*=================================*/
            char buff[1000];
            int fp2 = open("/home/liux7/Desktop/Linux/web/Web/time.txt",O_CREAT| O_WRONLY | O_APPEND,0644);

            (void)sprintf(buff, "%d request's totaltime:%lf\n per request use average time:%lf\n per request read socket average time:%lf\n \
per request write socket average time:%lf\n per request write log average time:%lf\n \
per request read html average time:%lf\n \n",               
                cnt,
                (*(double *)(tmpargv)),
                (*(double *)(tmpargv))/cnt,
                (*(double *) (tmpargv+1*sizeof(double)))/cnt,
                (*(double *) (tmpargv+2*sizeof(double)))/cnt,
                (*(double *) (tmpargv+3*sizeof(double)))/cnt,
                (*(double *) (tmpargv+4*sizeof(double)))/cnt);
            // (void)sprintf(buff, "%lf sR%lf sW%lf lt%lf rh%lf\n",ptime, socketRead/1000, socketWrite/1000, logtime/1000, readhtml/1000);
            (void)write(fp2, buff, strlen(buff));
            (void)close(fp2);
            /*===================================*/

			sem_post(psem);

			exit(1);
    	  } 
          else {   /* parent */
    	    (void)close(socketfd);
            /*=================================
            char buff[60];
            int fp2 = open("/home/liux7/Desktop/Linux/web/Web/time.txt",O_CREAT| O_WRONLY | O_APPEND,0644);
            (void)sprintf(buff, "ptime totaltime:%lf\n",  * (double *)totaltime);
            (void)write(fp2, buff, strlen(buff));
            (void)close(fp2);
            ===================================*/
    	  }
	  	  
	
		}
	}
}
