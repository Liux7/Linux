#ifndef _THREADPOOL_H_
#define _THREADPOOL_H_

/* queue status and conditional variable*/
typedef struct staconv
{
	pthread_mutex_t mutex;
	pthread_cond_t cond;
	int status;
}staconv;

/*Task*/
typedef struct task
{
	struct task* next;
	void (*function)(void* arg);
	void* arg;
}task;

/*Task Queue*/
typedef struct taskqueue
{
	pthread_mutex_t mutex;
	task *front;
	task *rear;
	staconv *has_jobs;
	int len;
}taskqueue;

/*Thread*/
typedef struct thread
{
	int id;
	pthread_t pthread;
	struct threadpool* pool;
} thread;

/*Thread Pool*/
typedef struct threadpool
{
	thread** threads;
	volatile int num_threads;
	volatile int num_working;
	pthread_mutex_t thcount_lock;
	pthread_cond_t threads_all_idle;
	taskqueue queue;
	volatile int is_alive;
}threadpool;

struct threadpool* initThreadPool(int num_threads);
void addTask2ThreadPool(struct threadpool* pool, struct task* curtask);
void waitThreadPool(struct threadpool* pool);
void destroyThreadPool(struct threadpool* pool);
int getNumofThreadWorking(struct threadpool* pool);
int create_thread(struct threadpool* pool, struct thread* pthread, int id);
void* thread_do(struct thread* pthread);
void init_taskqueue(struct taskqueue* queue);
void destroy_taskqueue(struct taskqueue* queue);
void push_taskqueue(struct taskqueue* queue, struct task* curtask);
struct task* take_taskqueue(struct taskqueue* queue);

#endif