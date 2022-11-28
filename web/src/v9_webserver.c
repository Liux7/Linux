#include <sys/prctl.h>
#include <stdbool.h>
#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h> 
#include <semaphore.h>

#define VERSION 23
#define BUFSIZE 8096
#define ERROR      42
#define LOG        44
#define FORBIDDEN 403
#define NOTFOUND  404

#ifndef SIGCLD
#   define SIGCLD SIGCHLD
#endif

#define NUM_THREADS 40
#define NUM_REQ 20
#define NUM_READ 2
#define NUM_SEND 20
#define NUM_OF_PAIR 10	// 桶长度
#define NUM_OF_BUCKET 10	// 桶数

struct {
	char* ext;
	char* filetype;
} extensions[] = {
  {"gif", "image/gif" },
  {"jpg", "image/jpg" },
  {"jpeg","image/jpeg"},
  {"png", "image/png" },
  {"ico", "image/ico" },
  {"zip", "image/zip" },
  {"gz",  "image/gz"  },
  {"tar", "image/tar" },
  {"htm", "text/html" },
  {"html","text/html" },
  {0,0}
};

/* 文件内容（长度 + 内容） */
typedef struct content
{
	int length;	    // 内容长度
	char* address;	// 内容起始地址
} content;

/* 文件信息对 （文件名 + 文件内容） */
typedef struct hashpair {
	char* key;		// 文件名
	content* cont;	// 内容项
	int times;
	struct hashpair* next;	// hash桶中指向下一个 hashpair
} hashpair;

/* hashtable，文件系统载体 */
typedef struct hashtable {
	hashpair** bucket;
	int num_bucket;         // 桶数量
	int num_pagefault;		// 缺页次数
	int num_ttlreq;			// 所有请求次数
	volatile int* locks;	// 对hash桶加锁
	volatile int lock;		// 对hash table加锁

} hashtable;

/* 队列状态和条件变量 */
typedef struct staconv {
	pthread_mutex_t mutex;		/* 辅助条件变量 cond */
	pthread_cond_t cond;		/* 等待和激发线程池中的线程 */
	int status;
}staconv;

/* 任务 */
typedef struct task {
	struct task* next;			/* 指向下一个任务 */
	void (*function)(void* arg);	/* 函数指针 */
	void* arg;					/* 函数参数指针 */
} task;

/* 任务队列 */
typedef struct taskqueue {
	pthread_mutex_t mutex;		/* 用于互斥读/写任务队列 */
	task* front;				/* 指向队首 */
	task* rear;				/* 指向队尾 */
	staconv* has_jobs;			/* 根据状态阻塞线程 */
	int len;					/* 任务队列中任务个数 */
} taskqueue;

/* 线程 */
typedef struct thread {
	int type;
	int id;						/* 线程 id */
	pthread_t pthread;			/* 封装的 POSIX 线程 */
	struct threadpool* pool;	/* 与线程池进行绑定 */
} thread;

/* 线程池 */
typedef struct threadpool {
	thread** threads;			/* 线程指针数组 */
	volatile int num_threads;	/* 线程池中线程数量 */
	volatile int num_working;	/* 目前正在工作的线程个数 */
	pthread_mutex_t thcount_lock;/* 线程池锁，用于修改上面两个变量  */
	pthread_cond_t threads_all_idle;/* 用于销毁线程的条件变量 */
	taskqueue queue;			/* 任务队列 */
	volatile bool is_alive;		/* 线程池是否存活 */
}threadpool;

typedef struct application {
	threadpool* reqpool;
	threadpool* readpool;
	threadpool* sendpool;
	hashtable* table;		// file cache
} application;

/* web()参数: 网络请求info */
typedef struct webparam {
	int hit;
	int fd;
	application* app;
} webparam;

typedef struct filename {
	int hit;
	char* fn;// filename
	char* fstr;
	int fd;// socket key word
	application* app;
} filename;

typedef struct msg {
	char* fc;//文件内容
	int fd;
	long len;
} msg;
/* nweb.log */
void logger(int type, char* s1, char* s2, int socket_fd);
hashpair* head;
static inline unsigned long hashString(unsigned char* str) {
	unsigned long hash = 5381;
	int c;

	while (c = *str++)
		hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
	return hash;
}

// helper for copying string keys and values
static inline char* copystring(char* value)
{
	char* copy = (char*)malloc(strlen(value) + 1);
	if (!copy) {
		printf("Unable to allocate string value %s\n", value);
		abort();
	}
	strcpy(copy, value);
	return copy;
}

// 判断两个 content是否相同，相同返回1，不同返回0
static inline int isEqualContent(content* cont1, content* cont2)
{
	/*printf("isequalcontent\n");*/
	if (cont1->length != cont2->length)
		return 0;
	if (cont1->address != cont2->address)
		return 0;
	return 1;
}


// Create hash table
hashtable* createHashTable(int num_bucket) {
	/*printf("creathashtable\n");*/
	hashtable* table = (hashtable*)malloc(sizeof(hashtable));
	if (NULL == table) {
		return NULL;
	}
	/* 创建 hash桶指针 */
	table->bucket = (hashpair**)malloc(num_bucket * sizeof(void*));
	if (NULL == table->bucket) {
		free(table);
		return NULL;
	}
	memset(table->bucket, 0, num_bucket * sizeof(void*));

	table->num_bucket = num_bucket;
	table->num_pagefault = 0;
	table->num_ttlreq = 0;

	/*初始化信号锁 */
	table->locks = (int*)malloc(num_bucket * sizeof(int));
	if (!table->locks) {
		free(table);
		return NULL;
	}
	memset((int*)&table->locks[0], 0, num_bucket * sizeof(int));
	/*printf("completehashtable\n");*/
	return table;
}

void freeHashTable(hashtable* table)  //释放哈希表
{
	/*printf("freehashtable\n");*/
	if (!table) return;
	hashpair* next;
	int i;
	for (i = 0; i < table->num_bucket; i++) {
		// 逐个桶释放
		hashpair* pair = table->bucket[i];
		while (pair) {
			next = pair->next;
			free(pair->key);
			free(pair->cont->address);
			free(pair->cont);
			free(pair);
			pair = next;
		}
	}
	free(table->bucket);
	/*#ifdef HASHTHREAD*/
	free(table->locks);
	/*#endif*/
	free(table);
}
/* 前提：调用者已获得锁 */
content* getContentByKey(hashtable* table, char* key)
{
	/*printf("getcontentbykey\n"); */
	int hash = hashString(key) % (table->num_bucket);
	int num = 0;
	hashpair* pair = table->bucket[hash];
	while (pair && num < 10)
	{
		printf("%s %d %d %s\n", pair->key, pair->times, hash, key);
		if (0 == strcmp(pair->key, key))
			return pair->cont;
		pair = pair->next;
		num++;
	}
	return NULL;
}

int get_file_size(char* key)
{
	/*printf("get file size\n"); */
	struct stat statbuff;
	if (stat(key, &statbuff) < 0) {
		return 0;
	}
	else {
		return (int)statbuff.st_size;
	}
}

/* 读取文件key，包装后返回 */
content* readFile2(char* key) {
	int l = get_file_size(key);
	/*printf("readfile2\n");*/
	content* tmp = malloc(sizeof(content));
	if (!tmp)
		return NULL;

	tmp->length = l + 1;
	tmp->address = calloc(l + 1, sizeof(char));
	if (!tmp->address) {
		free(tmp);
		return NULL;
	}
	int fd = open(key, O_RDONLY);
	if (fd == -1)   // 未打开文件
	{
		free(tmp->address);
		free(tmp);
		printf("return -1\n");
		return NULL;
	}
	else {
		read(fd, tmp->address, l);
		close(fd);
		/*printf("completely\n"); */
		return tmp;
	}

}

/* 前提：调用者已获得锁 */
int get_bucket_length(hashpair* pair) {
	int l = 0;
	while (pair && l < 10) {
		l++;
		pair = pair->next;
	}
	return l;
}
double sum = 0, avg;
/* FIFO算法实现的获取文件函数 */
content* getFileByFIFO(hashtable* table, char* key)  //key==f->fn
{
	int hash = hashString(key) % (table->num_bucket);  //找到哈希表头节点

	table->num_ttlreq++;   //请求次数加一

	content* tmp = getContentByKey(table, key);  //找到和key一致的哈希节点
	// 缓存中存在文件
	if (tmp)
		return tmp;
	// 缺页
	hashpair* pair = table->bucket[hash];
	content* cont = readFile2(key);    //读取key文件的内容
	// 统计
	int l = get_bucket_length(table->bucket[hash]);  //哈希表长
	printf("daozhelile\n");
	// 删除末尾pair
	if (l == NUM_OF_PAIR) {
		hashpair* prev = NULL;
		while (pair->next != NULL) {
			prev = pair;
			pair = pair->next;
		}
		if (!prev) {
			prev->next = NULL;
			free(pair->cont->address);
			free(pair->cont);
			free(pair);
		}
		// 记录缺页置换次数
		table->num_pagefault++;
	}
	// 头插
	pair = (hashpair*)malloc(sizeof(hashpair));
	pair->key = copystring(key);
	pair->cont = cont;
	pair->next = table->bucket[hash];	// 头插法
	table->bucket[hash] = pair;
	printf("%d %d %f\n", table->num_pagefault, table->num_ttlreq, (float)table->num_pagefault / (float)table->num_ttlreq);
	return cont;
}

/* 使用前提：调用者已加锁 */
void movePairToHead(hashtable* table, char* key)
{
	int hash = hashString(key) % (table->num_bucket);
	hashpair* pair = table->bucket[hash];
	// 不用移动
	if (!strcmp(key, pair->key)) {
		return;
	}
	// 找到位置、连接断裂处、头插
	hashpair* prev = NULL;
	while (pair->next != NULL)
	{
		if (!strcmp(pair->key, key)) {
			break;
		}
		prev = pair;
		pair = pair->next;
	}
	prev->next = pair->next;
	pair->next = table->bucket[hash];
	table->bucket[hash] = pair;
}

content* getFileByLRU(hashtable* table, char* key)
{
	int a[NUM_OF_PAIR];
	int num = 0, i, j, k;
	struct timeval start, end;
	double timeuse;
	pthread_mutex_t sendlock = PTHREAD_MUTEX_INITIALIZER;
	int hash = hashString(key) % (table->num_bucket); //该在哪个哈希目录下
	table->num_ttlreq++;  //请求次数加一
	gettimeofday(&start, NULL);  //计时开始

	content* tmp = getContentByKey(table, key);  //找缓存中有没有一样的文件
	// 缓存中存在文件
	hashpair* pair = table->bucket[hash];
	if (tmp) {  //有
		printf("cache has\n");
		if (!strcmp(key, pair->key)) {  //第一个就是
			printf("%d %d %f\n", table->num_pagefault, table->num_ttlreq, 1 - (float)table->num_pagefault / (float)table->num_ttlreq);
			gettimeofday(&end, NULL);   //要退出，计时结束
			pthread_mutex_lock(&sendlock);
			timeuse = (end.tv_sec - start.tv_sec) * 1000000.0 + end.tv_usec - start.tv_usec;
			sum = sum + timeuse;
			avg = sum / (double)table->num_ttlreq;
			printf("Average time is %lf\n", avg);//存取平均时间
			pthread_mutex_unlock(&sendlock);
			return tmp;
		}
		hashpair* prev = NULL;
		while (pair->next != NULL)  //不是第一个，往后找
		{
			if (!strcmp(pair->key, key)) {
				break;
			}
			prev = pair;
			pair = pair->next;
		}
		prev->next = pair->next;
		pair->next = table->bucket[hash];
		table->bucket[hash] = pair;
		printf("%d %d %f\n", table->num_pagefault, table->num_ttlreq, 1 - (float)table->num_pagefault / (float)table->num_ttlreq);
		gettimeofday(&end, NULL);  //计时结束
		pthread_mutex_lock(&sendlock);
		timeuse = (end.tv_sec - start.tv_sec) * 1000000.0 + end.tv_usec - start.tv_usec;
		sum = sum + timeuse;
		avg = sum / (double)table->num_ttlreq;
		printf("Average time is %lf\n", avg);  //存取平均时间
		pthread_mutex_unlock(&sendlock);
		return tmp;
	}
	table->num_pagefault++;
	content* cont = readFile2(key);
	int l = get_bucket_length(table->bucket[hash]);
	// 删除末尾pair
	if (l == NUM_OF_PAIR) {
		hashpair* prev = NULL;
		while (pair->next != NULL) {
			prev = pair;
			pair = pair->next;
		}
		if (!prev) {
			prev->next = NULL;
			free(pair->cont->address);
			free(pair->cont);
			free(pair);
		}
	}
	pair = (hashpair*)malloc(sizeof(hashpair));
	pair->key = copystring(key);
	pair->cont = cont;
	pair->next = table->bucket[hash];
	table->bucket[hash] = pair;
	printf("%d %d %f\n", table->num_pagefault, table->num_ttlreq, 1 - (float)table->num_pagefault / (float)table->num_ttlreq);
	gettimeofday(&end, NULL);  //计时结束
	pthread_mutex_lock(&sendlock);
	timeuse = (end.tv_sec - start.tv_sec) * 1000000.0 + end.tv_usec - start.tv_usec;
	sum = sum + timeuse;
	avg = sum / (double)table->num_ttlreq;
	printf("Average time is %lf\n", avg);
	pthread_mutex_unlock(&sendlock);
	return cont;
}

content* getFileByLFU(hashtable* table, char* key)
{
	int a[NUM_OF_PAIR];
	int num = 0, i, j, k, show = 0;
	struct timeval start, end;
	double timeuse;
	pthread_mutex_t sendlock = PTHREAD_MUTEX_INITIALIZER;
	int hash = hashString(key) % (table->num_bucket); //该在哪
	/*while (__sync_lock_test_and_set(&table->locks[hash], 1)); */
	table->num_ttlreq++;  //请求次数加一
	gettimeofday(&start, NULL);
	content* tmp = getContentByKey(table, key);
	hashpair* pair = table->bucket[hash];
	hashpair* cur = NULL;
	if (tmp) {    //找到了
		if (!strcmp(key, pair->key)) {
			pair->times++;
			printf("%d %d %f\n", table->num_pagefault, table->num_ttlreq, 1 - (float)table->num_pagefault / (float)table->num_ttlreq);
			printf("%s\n", key);
			/*__sync_synchronize();
			table->locks[hash] = 0;*/
			gettimeofday(&end, NULL);
			pthread_mutex_lock(&sendlock);
			timeuse = (end.tv_sec - start.tv_sec) * 1000000.0 + end.tv_usec - start.tv_usec;
			sum = sum + timeuse;
			avg = sum / (double)table->num_ttlreq;
			printf("Average time is %lf\n", avg);
			pthread_mutex_unlock(&sendlock);
			return tmp;
		}
		hashpair* prev = NULL, * p = table->bucket[hash], * q = NULL;
		while (pair->next != NULL)
		{
			if (!strcmp(pair->key, key)) {
				pair->times++;
				break;
			}
			prev = pair;
			pair = pair->next;
		}
		prev->next = pair->next;
		printf("cache have\n");
		while (p->times > pair->times && p) {   //缓存里有，从头往后找
			q = p;              //由于此节点times（次数）加一，所以要往前插入，这两行是找该插到哪里
			p = p->next;
		}
		pair->next = p;                  
		if (q) q->next = pair;     //次数大的往前排
		else table->bucket[hash] = pair;
		/*printf("%s\n", key); */
		printf("%d %d %f\n", table->num_pagefault, table->num_ttlreq, 1 - (float)table->num_pagefault / (float)table->num_ttlreq);
		/*__sync_synchronize();
		table->locks[hash] = 0;*/
		gettimeofday(&end, NULL);
		pthread_mutex_lock(&sendlock);
		timeuse = (end.tv_sec - start.tv_sec) * 1000000.0 + end.tv_usec - start.tv_usec;
		sum = sum + timeuse;
		avg = sum / (double)table->num_ttlreq;
		printf("Average time is %lf\n", avg);
		pthread_mutex_unlock(&sendlock);
		return tmp;
	}
	// 缺页
	content* cont = readFile2(key);
	cur = (hashpair*)malloc(sizeof(hashpair));
	cur->key = copystring(key);
	cur->cont = cont;
	cur->next = NULL;
	// 统计
	int l = get_bucket_length(table->bucket[hash]);
	if (l < NUM_OF_PAIR) {  //哈希没满
		/*printf("meiman\n"); */
		cur->times = 1;
		if (!pair) {  //该hash位置没有节点时，头插
			pair = (hashpair*)malloc(sizeof(hashpair));
			pair = table->bucket[hash];
			pair = cur;
			pair->next = table->bucket[hash];
			table->bucket[hash] = pair;
		}
		else { //有节点了，尾插
			while (pair->next != NULL && num <= 10) {
				printf("meimanelse %s %d %s\n", pair->key, pair->times, key);
				pair = pair->next;
				num++;
			}
			pair->next = cur;
		}
		table->num_pagefault++;  //缺页加一
		/*printf("insert directly completed\n"); */
	}
	else {   //哈希满了 删数
		/*printf("manle\n"); */
		int pnum;
		char* rem = NULL;
		hashpair* p = head, * q = NULL, * pre = NULL, * pr = NULL;
		if (p->next) rem = p->next->key;
		while (p->next != NULL) {   //垃圾堆里面找有没有当前key值页面cur
			if (!strcmp(p->key, key)) {  //若在垃圾堆链表里找到了，则这里就结束循环
				show = 1;
				p->times++;     //次数加一，因为此时正在访问该节点的内容
				pnum = p->times;
				break;
			}
			q = p;
			p = p->next;
			printf("head %s %d %s\n", p->key, p->times,rem);  //输出，能看到垃圾堆链表里的次数
		}
		if (show) {          //历史记录里有这个页面，p就是cur
			printf("youyemian\n");
			while (pair->next != NULL && num < 10) {  //在哈希链表里找到最后一个点
				pr = pair;
				pair = pair->next;
				num++;
			}
			//pr->next = pair->next;
			//q->next = p->next;
			if (pair->times <= pnum) {//哈希表最少的访问页面小于当前访问页面，可以放入,删除pair
				q = head->next;
				while (q->times > pair->times && q->next != NULL) {  //找到原哈希表中的值插入垃圾堆链表该插到哪里，
					                                                 //按次数降序排列
					pre = q;
					q = q->next;
				}
				//将pair(原哈希表中的）插入垃圾堆列表
				/*pr->next = NULL; */
				pair->next = q;
				if (pre) pre->next = pair;
				else if (!pre) head->next = pair;

				pair = table->bucket[hash]->next;
				num = 0;
				while (pair->times > p->times && pair->next && num < 10) {  //找到垃圾堆链表中的点（当前访问页）
					                                                       //该插到哈希表中的哪里，按次数降序排解
					pr = pair;
					pair = pair->next;
					num++;
					/*printf("manleshow %s %d %s\n", pair->key, pair->times, key);*/
				}
				/*printf("found2222222222\n"); */
				pr->next = p;    //将当前页面放入哈希表，当作缓存里的元素
				p->next = pair;
			}
			else {    //当前页面访问次数太少，不能放入
				p = head;
				while (p->times > cur->times && p->next) {  //插入垃圾堆链表，找该插到哪里
					printf("can't put111111111\n");
					q = p;
					p = p->next;
				}
				if (p->times <= cur->times) {  //cur(current)插入垃圾堆
					q->next = cur;
					cur->next = p;
				}
			}
		}
		else {  //show=0的情况，即此页面从未被访问过，且缓存也满了，那么插到末尾
			cur->times = 1;
			if (p) p->next = cur;  //p是遍历垃圾堆的while中的p指针，
			                         //若没有应该指向最后一个位置，直接插入即可
			else q->next = cur;
		}
		table->num_pagefault++;  //最后缺页加一
	}
	printf("%s\n", key);
	printf("%d %d %f\n", table->num_pagefault, table->num_ttlreq, 1 - (float)table->num_pagefault / (float)table->num_ttlreq);
	gettimeofday(&end, NULL);
	pthread_mutex_lock(&sendlock);
	timeuse = (end.tv_sec - start.tv_sec) * 1000000.0 + end.tv_usec - start.tv_usec;
	sum = sum + timeuse;
	avg = sum / (double)table->num_ttlreq;
	printf("Average time is %lf\n", avg);
	pthread_mutex_unlock(&sendlock);
	/*__sync_synchronize();
	table->locks[hash] = 0;*/
	return cont;
}

void push_taskqueue(taskqueue* queue, task* curTask) {
	if (NULL == curTask) return;
	pthread_mutex_lock(&queue->mutex);//写taskqueue
	if (queue->front == NULL && queue->rear == NULL) {
		curTask->next = NULL;
		queue->front = queue->rear = curTask;
	}
	else {
		curTask->next = NULL;
		queue->rear->next = curTask;
		queue->rear = curTask;
	}
	queue->len++;
	pthread_mutex_lock(&queue->has_jobs->mutex);//条件变量加锁
	queue->has_jobs->status = 1;
	pthread_cond_broadcast(&queue->has_jobs->cond); //激发阻塞的线程等待条件
	pthread_mutex_unlock(&queue->has_jobs->mutex);

	pthread_mutex_unlock(&queue->mutex);
}

void sendFile(void* msg2) {
	printf("sendfile\n");
	msg* m = (msg*)msg2;
	write(m->fd, m->fc, m->len);

	usleep(10000);
	close(m->fd);
	free(m);
}

void readFile(void* file)
{
	/*printf("readfile\n"); */
	filename* f = (filename*)file;
	// 发送信息写入log
	logger(LOG, "SEND", f->fn, f->hit);
	// 获取文件长度
	int len = get_file_size(f->fn);
	// 发送响应消息
	char buffer[BUFSIZE + 1];
	(void)sprintf(buffer, "HTTP/1.1 200 OK\nServer: nweb/%d.0\nContent-Length: %d\nConnection: close\nContent-Type: %s\n\n", VERSION, len, f->fstr);
	// 写入log
	logger(LOG, "Header", buffer, f->hit);
	(void)write(f->fd, buffer, strlen(buffer));

	msg* m = malloc(sizeof(msg));
	m->fd = f->fd;
	/*printf("before LFU\n"); */
	m->fc = getFileByLFU(f->app->table, f->fn)->address;
	m->len = len;

	free(f->fn);
	free(f->fstr);
	free(f);

	task* t = malloc(sizeof(task));
	t->function = &sendFile;
	t->arg = (void*)m;
	/*printf("qwq\n");*/
	push_taskqueue(&f->app->sendpool->queue, t);
}
void readReq(void* data)
{
	//1.读取文件信息
	//2.将 filename 和 fd 打包成filename 添加到 filename-queue
	int j, file_fd, buflen;
	long i, ret, len;
	char* fstr;
	char buffer[BUFSIZE + 1]; /* !!!!!!!! 不能是静态缓冲区 */
	webparam* param = (webparam*)data;
	int fd = param->fd;
	int hit = param->hit;

	ret = read(fd, buffer, BUFSIZE);   /* 从客户端读取请求消息 */

	if (ret == 0 || ret == -1) {  /* 读消息失败 */
		logger(FORBIDDEN, "failed to read browser request", "", fd);
		return;
	}
	else {
		if (ret > 0 && ret < BUFSIZE)  /* 设置有效字符串 */
			buffer[ret] = 0;
		else buffer[0] = 0;

		for (i = 0; i < ret; i++)  /* remove CF and LF characters */
			if (buffer[i] == '\r' || buffer[i] == '\n')
				buffer[i] = '*';
		// 请求信息l写入log
		logger(LOG, "request", buffer, hit);
		// method
		if (strncmp(buffer, "GET ", 4) && strncmp(buffer, "get ", 4)) {
			logger(FORBIDDEN, "Only simple GET operation supported", buffer, fd);
			close(fd);
			return;
		}
		for (i = 4; i < BUFSIZE; i++) { /* null terminate after the second space to ignore extra stuff */
			if (buffer[i] == ' ') { /* string is "GET URL " +lots of other stuff */
				buffer[i] = 0;
				break;
			}
		}
		// 不能使用 ..
		for (j = 0; j < i - 1; j++)   /* check for illegal parent directory use .. */
			if (buffer[j] == '.' && buffer[j + 1] == '.') {
				logger(FORBIDDEN, "Parent directory (..) path names not supported", buffer, fd);
				close(fd);
				return;
			}
		// 不包含有效文件名有效文件名，使用默认 /index.html
		if (!strncmp(&buffer[0], "GET /\0", 6) || !strncmp(&buffer[0], "get /\0", 6))
			(void)strcpy(buffer, "GET /index.html");
		// 根据预定义在extensionsh中的文件类型，检查请求的文件类型是否本服务器支持
		buflen = strlen(buffer);
		fstr = (char*)0;
		for (i = 0; extensions[i].ext != 0; i++) {
			len = strlen(extensions[i].ext);
			if (!strncmp(&buffer[buflen - len], extensions[i].ext, len)) {
				fstr = extensions[i].filetype;
				break;
			}
		}
		if (fstr == 0) {
			logger(FORBIDDEN, "file extension type not supported", buffer, fd);
			close(fd);
			return;
		}

		/* get file name ready for hash */
		filename* f = (filename*)malloc(sizeof(filename));
		f->hit = hit;
		f->fd = fd;
		int l = strlen(&buffer[5]);
		f->fn = (char*)malloc(sizeof(char) * (l + 1));
		strncpy(f->fn, &buffer[5], l);
		f->fn[l] = '\0';
		f->app = param->app;

		int l2 = strlen(fstr);
		f->fstr = (char*)malloc(sizeof(char) * (l2 + 1));   //扩展名
		strncpy(f->fstr, fstr, l2);
		f->fstr[l2] = '\0';

		task* t = (task*)malloc(sizeof(task));
		t->function = &readFile;
		t->arg = (void*)f;
		push_taskqueue(&param->app->readpool->queue, t);
		free(param);
	}
}

/* 初始化任务队列 */
void init_taskqueue(taskqueue* queue) {
	/*printf("init taskqueue\n");*/
	queue->front = NULL;
	queue->rear = NULL;
	queue->len = 0;
	pthread_mutex_init(&queue->mutex, NULL);//互斥读写任务队列
	/* staconv * has_jobs;  */
	queue->has_jobs = (staconv*)malloc(sizeof(struct staconv));
	pthread_mutex_init(&queue->has_jobs->mutex, NULL);
	pthread_cond_init(&queue->has_jobs->cond, NULL);//阻塞和唤醒线程池中线程
	queue->has_jobs->status = 0;//任务队列任务状态，0表示无任务
}

// 从哪个队列取任务
task* take_taskqueue(taskqueue* queue) {
	//从头部提取任务
	pthread_mutex_lock(&queue->mutex);
	task* t = NULL;
	if (queue->front == NULL && queue->rear == NULL) {
		pthread_mutex_unlock(&queue->mutex);
		return t;
	}
	else if (queue->front == queue->rear) {
		t = queue->front;
		queue->front = queue->rear = NULL;
	}
	else {
		t = queue->front;
		queue->front = queue->front->next;
	}
	queue->len--;
	if (queue->len == 0) {
		pthread_mutex_lock(&queue->has_jobs->mutex);//条件变量加锁
		queue->has_jobs->status = 0;
		pthread_mutex_unlock(&queue->has_jobs->mutex);
	}
	pthread_mutex_unlock(&queue->mutex);
	return t;
}

//销毁任务队列
void destory_taskqueue(taskqueue* queue) {
	/*printf("destroy taskqueue\n");*/
	pthread_mutex_lock(&queue->mutex);
	queue->len = 0;
	queue->has_jobs->status = 0;
	while (queue->front != queue->rear) {
		task* t = queue->front;
		queue->front = queue->front->next;
		free(t);
	}
	if (queue->front != NULL) free(queue->front);
	queue->front = queue->rear = NULL;
	free(queue->has_jobs);
	queue->has_jobs = NULL;
	pthread_mutex_unlock(&queue->mutex);
	free(queue);
}

void* thread_do(void* pth)
{
	/*printf("thread do\n");*/
	struct thread* pthread = (struct thread*)pth;
	char thread_name[128] = { 0 };
	sprintf(thread_name, "%d-thread-%d", pthread->type, pthread->id);

	prctl(PR_SET_NAME, thread_name);// 设置进程名

	/* 获得线程池 */
	threadpool* pool = pthread->pool;

	pthread_mutex_lock(&pool->thcount_lock);
	/* 在线程池初始化时，用于已经创建线程的计数 */
	pool->num_threads++;
	pthread_mutex_unlock(&pool->thcount_lock);


	/* 线程一直往返运行，直到 pool->is_alive=false */
	while (pool->is_alive) {

		/* 任务队列没有任务则阻塞 */
		// 使用 staconv * has_jobs 等待和激发
		pthread_mutex_lock(&pool->queue.has_jobs->mutex);
		while (pool->queue.has_jobs->status == 0) {
			pthread_cond_wait(&pool->queue.has_jobs->cond, &pool->queue.has_jobs->mutex); // 队列无任务，等待被激发
		}
		pthread_mutex_unlock(&pool->queue.has_jobs->mutex);

		if (pool->is_alive) {
			/* 对工作线程数量进行计数 */
			pthread_mutex_lock(&pool->thcount_lock);
			pool->num_working++;
			pthread_mutex_unlock(&pool->thcount_lock);

			void (*func)(void*);
			void* arg;
			task* curtask = take_taskqueue(&pool->queue);	/* take task from taskqueue */

			if (NULL != curtask) {
				// logger(LOG,"task_taskqueue_successful",thread_name,0);
				func = curtask->function;
				arg = curtask->arg;
				func(arg);//执行任务 web(void*)
				free(curtask);//释放任务
			}

			/* 线程已经将任务执行完成，需要更改工作线程数量 */
			pthread_mutex_lock(&pool->thcount_lock);
			pool->num_working--;
			pthread_mutex_lock(&pool->queue.mutex);
			if (0 == pool->num_working && pool->queue.len == 0) {	/* 任务全部完成，让阻塞在waitThreadPool上的线程继续运行 */
				pthread_cond_broadcast(&pool->threads_all_idle);		//条件激发，此时工作线程数=0，任务数=0 ==> 等待所有线程停止，进行线程池资源释放
			}
			pthread_mutex_unlock(&pool->queue.mutex);
			pthread_mutex_unlock(&pool->thcount_lock);

		}
	}
	pthread_mutex_lock(&pool->thcount_lock);
	/* 线程退出，要修改线程池中的数量 */
	pool->num_threads--;
	pthread_mutex_unlock(&pool->thcount_lock);

	return NULL;
}

/* 向线程池加入任务 */
void addTask2ThreadPool(threadpool* pool, task* curtask)
{
	//将任务加入队列
	//logger(LOG,"addTask2ThreadPool","",0);	
	push_taskqueue(&pool->queue, curtask);
}

/* 等待当前任务全部运行完 */
void waitThreadPool(threadpool* pool)
{
	/*printf("wait thread pool\n");*/
	pthread_mutex_lock(&pool->thcount_lock);
	while (pool->queue.len || pool->num_working) {
		pthread_cond_wait(&pool->threads_all_idle, &pool->thcount_lock);
	}
	pthread_mutex_unlock(&pool->thcount_lock);

}

/* 销毁线程池 */
void destoryThreadPool(threadpool* pool, int n)
{
	/*printf("destroythreadpool\n");*/
	//1.如果当前任务队列中有任务，需等待任务队列为空，并且运行线程执行任务后
	waitThreadPool(pool);	//len=0 && num_working=0
	//2.销毁任务队列
	pthread_mutex_lock(&pool->thcount_lock);
	pool->is_alive = false;
	pthread_mutex_unlock(&pool->thcount_lock);

	destory_taskqueue(&pool->queue);
	//3.销毁线程指针数组，并释放所有为线程池分配的内存
	int i = 0;
	for (; i < n; i++) free(pool->threads[i]);//free every thread
	free(pool->threads);//free array
	free(pool);//free pool
}

void destoryApp(application* app)
{
	/*printf("destroyapp\n"); */
	destoryThreadPool(app->reqpool, NUM_REQ);
	destoryThreadPool(app->readpool, NUM_READ);
	destoryThreadPool(app->sendpool, NUM_SEND);
	freeHashTable(app->table);
}

/* 获得当前线程池中正在运行线程的数量 */
int getNumofThreadWorking(threadpool* pool) {
	return pool->num_working;
}

/* 创建线程 */
int create_thread(struct threadpool* pool, struct thread* pthread, int id, int type)
{
	/*printf("create thread\n");*/
	pthread->pool = pool;
	pthread->id = id;
	pthread->type = type;
	pthread_create(&pthread->pthread, NULL, (void*)thread_do, (void*)pthread);//例程 thread_do, 参数 thread*
	pthread_detach(pthread->pthread);
	return 0;
}

/* 线程池初始化函数 */
struct threadpool* initThreadPool(int threads_num, int type) {
	/*printf("initthreadpool\n");*/
	// 创建线程池空间
	threadpool* pool = (threadpool*)malloc(sizeof(struct threadpool));
	pool->num_threads = 0;
	pool->num_working = 0;
	pthread_mutex_init(&(pool->thcount_lock), NULL);//互斥量,用来修改 num_threads和num_working
	pthread_cond_init(&(pool->threads_all_idle), NULL);//条件变量,用来销毁线程
	init_taskqueue(&(pool->queue));
	pool->is_alive = true;
	/* 存放`thread*`的数组 */
	pool->threads = (struct thread**)malloc(threads_num * sizeof(struct thread));//总共创建NUM_THREADS个线程
	for (int i = 0; i < threads_num; i++) {
		pool->threads[i] = (struct thread*)malloc(sizeof(struct thread));
		create_thread(pool, pool->threads[i], i, type);// i为线程id，循序执行，不必上锁
	}

	// 忙等待，直到所有线程创建完毕
	while (pool->num_threads != threads_num) {}

	//logger(LOG,"initThreadPool","",0);	
	return pool;
}

struct application* initApp() {
	struct application* app = (struct application*)malloc(sizeof(struct application));
	app->reqpool = initThreadPool(NUM_REQ, 1);
	app->readpool = initThreadPool(NUM_READ, 2);
	app->sendpool = initThreadPool(NUM_SEND, 3);
	app->table = createHashTable(NUM_OF_BUCKET);
	return app;
}
void inithashpair()
{
	head = (struct hashpair*)malloc(sizeof(struct hashpair));
	if (!head) {
		printf("error head\n");
		exit(1);
	}
	head->cont = "aaaaa";
	head->key = "aaaaaaa";
	head->times = 0;
	head->next = NULL;
}

int main(int argc, char** argv)
{
	int i, port, pid, listenfd, socketfd, hit;
	socklen_t length;
	static struct sockaddr_in cli_addr; /* static = initialised to zeros */
	static struct sockaddr_in serv_addr; /* static = initialised to zeros */
	// 解析b命令行参数
	if (argc < 3 || argc > 3 || !strcmp(argv[1], "-?")) {
		(void)printf("hint: nweb Port-Number Top-Directory\t\tversion %d\n\n"
			"\tnweb is a small and very safe mini web server\n"
			"\tnweb only servers out file/web pages with extensions named below\n"
			"\t and only from the named directory or its sub-directories.\n"
			"\tThere is no fancy features = safe and secure.\n\n"
			"\tExample: nweb 8181 /home/nwebdir &\n\n"
			"\tOnly Supports:", VERSION);
		for (i = 0; extensions[i].ext != 0; i++)
			(void)printf(" %s", extensions[i].ext);
		(void)printf("\n\tNot Supported: URLs including \"..\", Java, Javascript, CGI\n"
			"\tNot Supported: directories / /etc /bin /lib /tmp /usr /dev /sbin \n"
			"\tNo warranty given or implied\n\tNigel Griffiths nag@uk.ibm.com\n");
		exit(0);
	}
	if (!strncmp(argv[2], "/", 2) || !strncmp(argv[2], "/etc", 5) ||
		!strncmp(argv[2], "/bin", 5) || !strncmp(argv[2], "/lib", 5) ||
		!strncmp(argv[2], "/tmp", 5) || !strncmp(argv[2], "/usr", 5) ||
		!strncmp(argv[2], "/dev", 5) || !strncmp(argv[2], "/sbin", 6))
	{
		(void)printf("ERROR: Bad top directory %s, see nweb -?\n", argv[2]);
		exit(3);
	}
	if (chdir(argv[2]) == -1) {
		(void)printf("ERROR: Can't Change to directory %s\n", argv[2]);
		exit(4);
	}
	(void)signal(SIGCLD, SIG_IGN); // ignore child death 
	(void)signal(SIGHUP, SIG_IGN); // ignore terminal hangups 
	logger(LOG, "nweb starting", argv[1], getpid());

	// 建立服务端侦听socket
	if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
		logger(ERROR, "system call", "socket", 0);

	port = atoi(argv[1]);
	if (port < 0 || port >60000)
		logger(ERROR, "Invalid port number (try 1->60000)", argv[1], 0);
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	serv_addr.sin_port = htons(port);
	if (bind(listenfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0)
		logger(ERROR, "system call", "bind", 0);
	if (listen(listenfd, 64) < 0)
		logger(ERROR, "system call", "listen", 0);
	//struct threadpool * pool = initThreadPool();//不传递参数，用宏常量
	struct application* app = initApp();
	inithashpair();
	for (hit = 1; ; hit++) {
		length = sizeof(cli_addr);
		// gettimeofday(&ts,NULL) ; //开始
		if ((socketfd = accept(listenfd, (struct sockaddr*)&cli_addr, &length)) < 0)
		{
			logger(ERROR, "system call", "accept", 0);
			continue;
		}
		/*printf("main for\n");*/
		webparam* param = malloc(sizeof(webparam));
		param->fd = socketfd;
		param->hit = hit;
		param->app = app;

		task* curTask = malloc(sizeof(struct task));
		curTask->function = &readReq;
		curTask->arg = (void*)param;
		addTask2ThreadPool(app->reqpool, curTask);
		// gettimeofday(&end, NULL); //结束
		// pthread_mutex_lock(&sendlock);
		// // timeuse = (end.tv_sec - start.tv_sec) * 1000000.0 + end.tv_usec - start.tv_usec;
		// // sum = sum + timeuse;
		// // avg = sum / (double)table->num_ttlreq;
		// printf("Average time is %lf\n", avg);
		// pthread_mutex_unlock(&sendlock);
	}
	destoryApp(app);
}

void logger(int type, char* s1, char* s2, int socket_fd)
{
	int fd;
	char logbuffer[BUFSIZE * 2];

	// 获取时间信息
	struct tm* ptr;
	time_t lt;
	lt = time(NULL);
	ptr = localtime(&lt);
	char timebuf[48] = { 0 };
	strftime(timebuf, sizeof(timebuf), "%H:%M:%S %a %d/%m/%Y", ptr);

	/*将消息写入logger，或直接将消息通过socket通道返回给客户端*/

	switch (type) {
	case ERROR: (void)sprintf(logbuffer, "[%s] ERROR: %s:%s Errno=%d exiting pid=%d\n", timebuf, s1, s2, errno, getpid());
		break;
	case FORBIDDEN:
		(void)write(socket_fd, "HTTP/1.1 403 Forbidden\nContent-Length: 185\nConnection: close\nContent-Type: text/html\n\n<html><head>\n<title>403 Forbidden</title>\n</head><body>\n<h1>Forbidden</h1>\nThe requested URL, file type or operation is not allowed on this simple static file webserver.\n</body></html>\n", 271);
		(void)sprintf(logbuffer, "[%s] FORBIDDEN: %s:%s:%d\n", timebuf, s1, s2, socket_fd);
		break;
	case NOTFOUND:
		(void)write(socket_fd, "HTTP/1.1 404 Not Found\nContent-Length: 136\nConnection: close\nContent-Type: text/html\n\n<html><head>\n<title>404 Not Found</title>\n</head><body>\n<h1>Not Found</h1>\nThe requested URL was not found on this server.\n</body></html>\n", 224);
		(void)sprintf(logbuffer, "[%s] NOT FOUND: %s:%s:%d\n", timebuf, s1, s2, socket_fd);
		break;

	case LOG: (void)sprintf(logbuffer, "[%s] INFO: %s:%s:%d", timebuf, s1, s2, socket_fd); break;
	}
	/* No checks here, nothing can be done with a failure anyway */
	if ((fd = open("nweb.log", O_CREAT | O_WRONLY | O_APPEND, 0644)) >= 0) {
		(void)write(fd, logbuffer, strlen(logbuffer));
		(void)write(fd, "\n", 1);
		(void)close(fd);
	}

	//if(type == NOTFOUND || type == FORBIDDEN) pthread_exit((void*)3);
	//if(type == ERROR) exit(3);
}