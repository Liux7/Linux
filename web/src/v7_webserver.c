#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <sys/stat.h>
#include <sys/prctl.h>
#include <stdbool.h>
#include <sys/time.h>
#define VERSION 23
#define BUFSIZE 8096
#define ERROR      42
#define LOG        44
#define FORBIDDEN 403
#define NOTFOUND  404

#ifndef SIGCLD
#define SIGCLD SIGCHLD
#endif


int createthreadcount=0;//总线程数
double time_block=0;//总阻塞时间
double time_active=0;//总活跃时间
struct {
    char *ext;
    char *filetype;
} extensions[] = {
        {"gif",  "image/gif"},
        {"jpg",  "image/jpg"},
        {"jpeg", "image/jpeg"},
        {"png",  "image/png"},
        {"ico",  "image/ico"},
        {"zip",  "image/zip"},
        {"gz",   "image/gz"},
        {"tar",  "image/tar"},
        {"htm",  "text/html"},
        {"html", "text/html"},
        {0,      0}};


unsigned long get_file_size(const char *path) {
    unsigned long filesize = -1;
    struct stat statbuff;
    if (stat(path, &statbuff) < 0) {
        return filesize;
    } else {
        filesize = statbuff.st_size;
    }
    return filesize;
}

void logger(int type, char *s1, char *s2, int socket_fd) {
    int fd;
    char logbuffer[BUFSIZE * 2];

    switch (type) {
        case ERROR:
            (void) sprintf(logbuffer, "ERROR: %s:%s Errno=%d exiting pid=%d", s1, s2, errno, getpid());
            break;
        case FORBIDDEN:
            (void) write(socket_fd,
                         "HTTP/1.1 403 Forbidden\nContent-Length: 185\nConnection: close\nContent-Type: text/html\n\n<html><head>\n<title>403 Forbidden</title>\n</head><body>\n<h1>Forbidden</h1>\nThe requested URL, file type or operation is not allowed on this simple static file webserver.\n</body></html>\n",
                         271);
            (void) sprintf(logbuffer, "FORBIDDEN: %s:%s", s1, s2);
            break;
        case NOTFOUND:
            (void) write(socket_fd,
                         "HTTP/1.1 404 Not Found\nContent-Length: 136\nConnection: close\nContent-Type: text/html\n\n<html><head>\n<title>404 Not Found</title>\n</head><body>\n<h1>Not Found</h1>\nThe requested URL was not found on this server.\n</body></html>\n",
                         224);
            (void) sprintf(logbuffer, "NOT FOUND: %s:%s", s1, s2);
            break;
        case LOG:
            (void) sprintf(logbuffer, " INFO: %s:%s:%d", s1, s2, socket_fd);
            break;
    }
    /* No checks here, nothing can be done with a failure anyway */
    if ((fd = open("webserver.log", O_CREAT | O_WRONLY | O_APPEND, 0644)) >= 0) {
        (void) write(fd, logbuffer, strlen(logbuffer));
        (void) write(fd, "\n", 1);
        (void) close(fd);
    }
    //if (type == ERROR || type == NOTFOUND || type == FORBIDDEN) exit(3);
}

typedef struct {
    int hit;
    int fd;
    int file_fd;//新增文件描述符属性，用以传递文件信息
    char buffer[BUFSIZE + 1];//存文件名
} webparam;

typedef struct staconv {
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    int status;
} staconv;

typedef struct task {
    struct task *next;

    void (*function)(void *arg);

    void *arg;
} task;

typedef struct taskqueue {
    pthread_mutex_t mutex;
    task *front;
    task *rear;
    staconv *has_jobs;
    int len;
} taskqueue;

typedef struct thread {
    int id;
    pthread_t pthread;
    struct threadpool *pool;
} thread;

typedef struct threadpool {
    thread **threads;
    volatile int num_threads;
    volatile int num_working;
    pthread_mutex_t thcount_lock;
    pthread_cond_t threads_all_idle;
    taskqueue queue;
    volatile bool is_alive;
} threadpool;

threadpool *readmsg_pool;
threadpool *readfile_pool;
threadpool *sendmsg_pool;

//添加任务
void push_taskqueue(taskqueue *queue, task *curtask) 
{
    pthread_mutex_lock(&queue->mutex);
    if (queue->len == 0) 
	{      //队列为空队首、队尾指向当前任务
        queue->front = curtask;
        queue->rear = curtask;
    } else {      //队列非空队尾next指向当前任务，队尾指向当前任务
        queue->rear->next = curtask;
        queue->rear = curtask;
    }
    queue->len++;   //长度加1
    pthread_cond_signal(&queue->has_jobs->cond);    //唤醒阻塞在在条件变量上的线程
    queue->has_jobs->status = 1;    //任务队列状态为有任务
    pthread_mutex_unlock(&queue->mutex);
}

//取任务
task *take_taskqueue(taskqueue *queue) {
    pthread_mutex_lock(&queue->mutex);
    task *curtask = queue->front;   //取队首若为空则为NULL
    if (queue->len > 0) {   //队列非空队首指向下一个
        queue->front = queue->front->next;
        queue->len--;
    }
    if (queue->len == 0) {  //取任务后队列为空
        queue->front = NULL;
        queue->rear = NULL;
        queue->has_jobs->status = 0;    //设置任务队列状态为无任务
    }
    pthread_mutex_unlock(&queue->mutex);
    return curtask;
}

//初始化任务队列
void init_taskqueue(taskqueue *queue) {
    queue->front = NULL;
    queue->rear = NULL;
    queue->len = 0;
    pthread_mutex_init(&queue->mutex, NULL);
    queue->has_jobs = (staconv *) malloc(sizeof(staconv));
    //初始化互斥量和条件变量
    pthread_mutex_init(&(queue->has_jobs->mutex), NULL);
    pthread_cond_init(&(queue->has_jobs->cond), NULL);
    queue->has_jobs->status = 0;    //设置无任务状态
}

//销毁任务队列
void destory_taskqueue(taskqueue *queue) {
    pthread_mutex_lock(&queue->mutex);
    //释放任务
    while (queue->len > 0) {
        task *curtask = take_taskqueue(queue);
        free(curtask);
    }
    //销毁互斥量和条件变量
    pthread_mutex_destroy(&queue->has_jobs->mutex);
    pthread_cond_destroy(&queue->has_jobs->cond);
    free(queue->has_jobs);
    pthread_mutex_unlock(&queue->mutex);
    //销毁队列互斥锁
    pthread_mutex_destroy(&queue->mutex);
}

//线程运行的逻辑函数
void *thread_do(struct thread *pthread) {
    char thread_name[128] = {0};
    sprintf(thread_name, "thread-pool-%d", pthread->id);

    prctl(PR_SET_NAME, thread_name);
    //获得线程池
    threadpool *pool = pthread->pool;
    //在线程初始化时，用于已经创建线程的计数，执行pool->num_threads++;
    pthread_mutex_lock(&pool->thcount_lock);
	
    pool->num_threads++;
	createthreadcount++;
	
    pthread_mutex_unlock(&pool->thcount_lock);
    //线程循环直到pool->is_alive为false
    while (pool->is_alive) {
        //如果任务队列中还有任务，则继续运行，否则阻塞


        pthread_mutex_lock(&pool->queue.has_jobs->mutex);
        //阻塞时间
        struct timeval t1, t2;
        gettimeofday(&t1, NULL);

        while (pool->queue.has_jobs->status == 0)   //没有任务阻塞
            pthread_cond_wait(&pool->queue.has_jobs->cond, &pool->queue.has_jobs->mutex);
        gettimeofday(&t2, NULL);

        double time = (t2.tv_sec - t1.tv_sec) * 1000 + (t2.tv_usec - t1.tv_usec) / 1000.0;
        time_block+=time;

        pthread_mutex_unlock(&pool->queue.has_jobs->mutex);
		//活跃时间
        gettimeofday(&t1, NULL);
        if (pool->is_alive) {
            
           

            //执行到此位置，表示线程在工作，需要对工作线程数量进行计数
            pthread_mutex_lock(&pool->thcount_lock);
            pool->num_working++;
            pthread_mutex_unlock(&pool->thcount_lock);
            //取队首任务执行
            void (*func)(void *);
            void *arg;
            //提取并删除任务
            task *curtask = take_taskqueue(&pool->queue);
            if (curtask) {
                func = curtask->function;
                arg = curtask->arg;
                func(arg);  //执行任务
                free(curtask);  //释放任务
            }

           
        }
		gettimeofday(&t2, NULL);
        time = (t2.tv_sec - t1.tv_sec) * 1000 + (t2.tv_usec - t1.tv_usec) / 1000.0;
        time_active+=time;
		
        //线程任务完成，更改工作线程数量。工作线程为0，任务全部完成，让阻塞线程继续运行
        pthread_mutex_lock(&pool->thcount_lock);
        pool->num_working--;
        if (pool->num_working == 0) {
            pthread_cond_signal(&pool->threads_all_idle);
        }
        pthread_mutex_unlock(&pool->thcount_lock);
    }
    //线程将要退出,更改线程数量
    pthread_mutex_lock(&pool->thcount_lock);
    pool->num_threads--;
    pthread_mutex_unlock(&pool->thcount_lock);
    //
    return NULL;
}

//创建线程
int create_thread(struct threadpool *pool, struct thread **pthread, int id) 
{
    *pthread = (struct thread *) malloc(sizeof(struct thread));
    if (pthread == NULL) {
        perror("creat_thread():Could not allocate memory for thread\n");
        return -1;
    }
    (*pthread)->pool = pool;
    (*pthread)->id = id;
	
	//计数总线程数
	//pthread_mutex_lock(&tcount_mutex);
	//createthreadcount++;
	//pthread_mutex_unlock(&tcount_mutex);
	
	
    pthread_create(&(*pthread)->pthread, NULL, (void *) thread_do, (*pthread));
    pthread_detach((*pthread)->pthread);
    return 0;
}

//初始化线程函数
struct threadpool *initThreadPool(int num_threads) {
    //创建线程池空间
    threadpool *pool;
    pool = (threadpool *) malloc(sizeof(struct threadpool));
    pool->num_threads = 0;
    pool->num_working = 0;
    pool->is_alive = true;
    //初始化互斥量和条件变量
    pthread_mutex_init(&(pool->thcount_lock), NULL);
    pthread_cond_init(&pool->threads_all_idle, NULL);
    //初始化任务队列
    init_taskqueue(&pool->queue);
    //创建线程数组
    pool->threads = (struct thread **) malloc(num_threads * sizeof(struct thread *));
    //创建线程
    int i;
    for (i = 0; i < num_threads; ++i) {
        create_thread(pool, &pool->threads[i], i);
    }
    while (pool->num_threads != num_threads) {

    }
    return pool;
}

//向线程池中添加任务
void addTaskThreadPool(threadpool *pool, task *curtask) {
    //将任务加入队列
    push_taskqueue(&pool->queue, curtask);
}

//等待当前任务全部运行完成
void waitThreadPool(threadpool *pool) {
    pthread_mutex_lock(&pool->thcount_lock);
    while (pool->queue.len || pool->num_working) {
        pthread_cond_wait(&pool->threads_all_idle, &pool->thcount_lock);
    }
    pthread_mutex_unlock(&pool->thcount_lock);
}

//销毁线程池
void destoryThreadPool(threadpool *pool) {
    //如果当前任务队列中有任务，需要等待任务队列为空，并且线程执行完任务后
    while (pool->queue.has_jobs->status) {
        waitThreadPool(pool);
    }
    //销毁任务队列
    destory_taskqueue(&pool->queue);
    //销毁线程指针数组，并释放所有为线程池分配的内存
    int i;
    for (i = 0; i < pool->num_threads; ++i) {
        free(pool->threads[i]);
    }
    free(pool->threads);
    //销毁互斥量和条件变量
    pthread_mutex_destroy(&pool->thcount_lock);
    pthread_cond_destroy(&pool->threads_all_idle);
    free(pool);
}

//获取当前运行线程数
int getNumOfThreadWorking(threadpool *pool) {
    return pool->num_working;
}




void *web_sendmsg(void *data) {
    int fd;
    int hit;

    int j, file_fd, buflen;
    long i, ret, len;
    char buffer[BUFSIZE + 1]; /* static so zero filled */
    webparam *param = (webparam *) data;
    fd = param->fd;
    hit = param->hit;
    file_fd = param->file_fd;
    /* send file in 8KB block - last block may be smaller */
    while ((ret = read(file_fd, buffer, BUFSIZE)) > 0) {
        (void) write(fd, buffer, ret);
    }
    usleep(10000);  /* allow socket to drain before signalling the socket is closed */
    close(file_fd);

    close(fd);
    //释放内存
    free(param);
}

void *web_readfile(void *data) {
    int fd;
    int hit;

    int j, file_fd, buflen;
    long i, ret, len;
    char *fstr;
    char buffer[BUFSIZE + 1]; /* static so zero filled */
    webparam *param = (webparam *) data;
    fd = param->fd;
    hit = param->hit;
    memcpy(buffer, param->buffer, sizeof(param->buffer));
    /* work out the file type and check we support it */
    buflen = strlen(buffer);
    fstr = (char *) 0;
    for (i = 0; extensions[i].ext != 0; i++) {
        len = strlen(extensions[i].ext);
        if (!strncmp(&buffer[buflen - len], extensions[i].ext, len)) {
            fstr = extensions[i].filetype;
            break;
        }
    }
    if (fstr == 0) logger(FORBIDDEN, "file extension type not supported", buffer, fd);

    if ((file_fd = open(&buffer[5], O_RDONLY)) == -1) {  /* open the file for reading */
        logger(NOTFOUND, "failed to open file", &buffer[5], fd);
    }
    logger(LOG, "SEND", &buffer[5], hit);
    len = (long) lseek(file_fd, (off_t) 0, SEEK_END); /* lseek to the file end to find the length */
    (void) lseek(file_fd, (off_t) 0, SEEK_SET); /* lseek back to the file start ready for reading */
    (void) sprintf(buffer,
                   "HTTP/1.1 200 OK\nServer: nweb/%d.0\nContent-Length: %ld\nConnection: close\nContent-Type: %s\n\n",
                   VERSION, len, fstr); /* Header + a blank line */
    logger(LOG, "Header", buffer, hit);
    (void) write(fd, buffer, strlen(buffer));


    webparam *param_sendmsg = malloc(sizeof(webparam));
    param_sendmsg->hit = hit;
    param_sendmsg->fd = fd;
    param_sendmsg->file_fd = file_fd;
    memcpy(param_sendmsg->buffer, buffer, sizeof(buffer));//将buffer内容即文件名复制给待传递的webparam
   
    task *Task = (task *) malloc(sizeof(task)); //建一个sendfile任务
    Task->function = (void *) web_sendmsg;//入口函数为sendmsg
    Task->arg = (void *) param_sendmsg;
    addTaskThreadPool(sendmsg_pool, Task);//将sendfile任务加到发送文件线程池中，也就是加入msg queue中

    free(param);
}

void *web_readmsg(void *data) {
    int fd;
    int hit;

    int j;
    long i, ret;
    char buffer[BUFSIZE + 1]; /* static so zero filled */
    webparam *param = (webparam *) data;
    fd = param->fd;
    hit = param->hit;

    ret = read(fd, buffer, BUFSIZE);   /* read Web request in one go */
    if (ret == 0 || ret == -1) {  /* read failure stop now */
        logger(FORBIDDEN, "failed to read browser request", "", fd);
    } else {
        if (ret > 0 && ret < BUFSIZE)  /* return code is valid chars */
            buffer[ret] = 0;    /* terminate the buffer */
        else buffer[0] = 0;
        for (i = 0; i < ret; i++)  /* remove CF and LF characters */
            if (buffer[i] == '\r' || buffer[i] == '\n')
                buffer[i] = '*';
        logger(LOG, "request", buffer, hit);
        if (strncmp(buffer, "GET ", 4) && strncmp(buffer, "get ", 4)) {
            logger(FORBIDDEN, "Only simple GET operation supported", buffer, fd);
        }
        for (i = 4; i < BUFSIZE; i++) { /* null terminate after the second space to ignore extra stuff */
            if (buffer[i] == ' ') { /* string is "GET URL " +lots of other stuff */
                buffer[i] = 0;
                break;
            }
        }
        for (j = 0; j < i - 1; j++)   /* check for illegal parent directory use .. */
            if (buffer[j] == '.' && buffer[j + 1] == '.') {
                logger(FORBIDDEN, "Parent directory (..) path names not supported", buffer, fd);
            }
        if (!strncmp(&buffer[0], "GET /\0", 6) ||
            !strncmp(&buffer[0], "get /\0", 6)) /* convert no filename to index file */
            (void) strcpy(buffer, "GET /index.html");

        webparam *param_readfile = malloc(sizeof(webparam));
        param_readfile->hit = hit;
        param_readfile->fd = fd;
        memcpy(param_readfile->buffer, buffer, sizeof(buffer));
        
        task *Task = (task *) malloc(sizeof(task));//建一个readfile任务
        Task->function = (void *) web_readfile;//入口函数为readfile ，在上面
        Task->arg = (void *) param_readfile;
        addTaskThreadPool(readfile_pool, Task);//将readfile任务加到读文件线程池中，也就是加入filename queue中
    }
}

void *monitor(void *args) //监测参数
{
   int max_readmsg,max_readfile,max_sendmsg;
   int count=1;//检测次数
   int min_readmsg,min_readfile,min_sendmsg;
    while (true) 
	{
        sleep(3);//每三秒输出一次监控结果
		if(count==1)
		{
			max_readmsg=readmsg_pool->num_working;
			max_readfile=readfile_pool->num_working;
			max_sendmsg=sendmsg_pool->num_working;
			
			min_readmsg=max_readmsg;
			min_readfile=max_readfile;
			min_sendmsg=max_sendmsg;
		}
		else
		{
			if(readmsg_pool->num_working>max_readmsg)max_readmsg=readmsg_pool->num_working;
			if(readfile_pool->num_working>max_readfile)max_readfile=readfile_pool->num_working;
			if(sendmsg_pool->num_working>max_sendmsg)max_sendmsg=sendmsg_pool->num_working;
			
			if(readmsg_pool->num_working<min_readmsg)min_readmsg=readmsg_pool->num_working;
			if(readfile_pool->num_working<min_readfile)min_readfile=readfile_pool->num_working;
			if(sendmsg_pool->num_working<min_sendmsg)min_sendmsg=sendmsg_pool->num_working;
		}
		printf("第%d次监测结果如下：\n",count);
		printf("线程平均活跃时间为：%lf,线程平均阻塞时间为：%lf\n",time_active/createthreadcount,time_block/createthreadcount);
		printf("readmsg pool 最高活跃：%d,最低活跃：%d\n",max_readmsg,min_readmsg);
		printf("readfile pool 最高活跃：%d,最低活跃：%d\n",max_readfile,min_readfile);
		printf("sendmsg pool 最高活跃：%d,最低活跃：%d\n",max_sendmsg,min_sendmsg);
		printf("------------------------------------------------------------------\n");
		count++;
    }
	
}

int main(int argc, char **argv) {
    int i, port, pid, listenfd, socketfd, hit;
    socklen_t length;
    static struct sockaddr_in cli_addr; /* static = initialised to zeros */
    static struct sockaddr_in serv_addr; /* static = initialised to zeros */

    if (argc < 3 || argc > 3 || !strcmp(argv[1], "-?")) {
        (void) printf("hint: nweb Port-Number Top-Directory\t\tversion %d\n\n"
                      "\tnweb is a small and very safe mini web server\n"
                      "\tnweb only servers out file/web pages with extensions named below\n"
                      "\t and only from the named directory or its sub-directories.\n"
                      "\tThere is no fancy features = safe and secure.\n\n"
                      "\tExample: nweb 8181 /home/nwebdir &\n\n"
                      "\tOnly Supports:", VERSION);
        for (i = 0; extensions[i].ext != 0; i++)
            (void) printf(" %s", extensions[i].ext);

        (void) printf("\n\tNot Supported: URLs including \"..\", Java, Javascript, CGI\n"
                      "\tNot Supported: directories / /etc /bin /lib /tmp /usr /dev /sbin \n"
                      "\tNo warranty given or implied\n\tNigel Griffiths nag@uk.ibm.com\n");
        exit(0);
    }
    if (!strncmp(argv[2], "/", 2) || !strncmp(argv[2], "/etc", 5) ||
        !strncmp(argv[2], "/bin", 5) || !strncmp(argv[2], "/lib", 5) ||
        !strncmp(argv[2], "/tmp", 5) || !strncmp(argv[2], "/usr", 5) ||
        !strncmp(argv[2], "/dev", 5) || !strncmp(argv[2], "/sbin", 6)) {
        (void) printf("ERROR: Bad top directory %s, see nweb -?\n", argv[2]);
        exit(3);
    }
    if (chdir(argv[2]) == -1) {
        (void) printf("ERROR: Can't Change to directory %s\n", argv[2]);
        exit(4);
    }
    /* setup the network socket */
    if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        logger(ERROR, "system call", "socket", 0);
    port = atoi(argv[1]);
    if (port < 0 || port > 60000)
        logger(ERROR, "Invalid port number (try 1->60000)", argv[1], 0);

    //初始化线程属性，为分离状态
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    pthread_t pth;

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(port);
    if (bind(listenfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
        logger(ERROR, "system call", "bind", 0);
    if (listen(listenfd, 64) < 0)
        logger(ERROR, "system call", "listen", 0);

	//对已经创建的三个线程池初始化
    readmsg_pool = initThreadPool(200);
    readfile_pool = initThreadPool(200);
    sendmsg_pool = initThreadPool(200);

    
	
    for (hit = 1;; hit++) 
	{
        length = sizeof(cli_addr);
        if ((socketfd = accept(listenfd, (struct sockaddr *) &cli_addr, &length)) < 0)
            logger(ERROR, "system call", "accept", 0);
        webparam *param = malloc(sizeof(webparam));
        param->hit = hit;
        param->fd = socketfd;
       
        task *Task = (task *) malloc(sizeof(task)); //创建一个任务
        Task->function = (void *) web_readmsg;//新任务先执行读信息功能
        Task->arg = (void *) param;
        addTaskThreadPool(readmsg_pool, Task);  //将新建任务加到读信息线程池中

        if (hit==1)
       {
        pthread_t m;
        pthread_create(&m, NULL, monitor, NULL);//新开一个线程，监控参数
       }
    }

}