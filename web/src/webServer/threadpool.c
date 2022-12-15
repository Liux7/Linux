#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
//#include <sys/prctl.h>
#include "threadpool.h"

struct threadpool* initThreadPool(int num_threads)
{
	threadpool* pool;
	pool = (threadpool*)malloc(sizeof(struct threadpool));
	pool->num_threads = 0;
	pool->num_working = 0;
	pool->is_alive = 1;
	pthread_mutex_init(&(pool->thcount_lock), NULL);
	pthread_cond_init(&(pool->threads_all_idle), NULL);
	init_taskqueue(&(pool->queue));
	pool->threads = (struct thread**)malloc(num_threads * sizeof(struct thread*));
	for (int i = 0; i < num_threads; ++i)
	{
		create_thread(pool, pool->threads[i], i);
	}
	while(pool->num_threads != num_threads){}
	return pool;
}

void addTask2ThreadPool(struct threadpool* pool, struct task* curtask)
{
	push_taskqueue(&(pool->queue), curtask);
}

void waitThreadPool(struct threadpool* pool)
{
	pthread_mutex_lock(&(pool->thcount_lock));
	while(pool->queue.len || pool->num_working)
	{
		pthread_cond_wait(&(pool->threads_all_idle), &(pool->thcount_lock));
	}
	pthread_mutex_unlock(&(pool->thcount_lock));
}

void destroyThreadPool(struct threadpool* pool)
{
	waitThreadPool(pool);
	pool->is_alive = 0;
	pthread_cond_broadcast(&(pool->queue.has_jobs->cond));
	while(pool->num_threads != 0){}
	destroy_taskqueue(&(pool->queue));
	pthread_mutex_destroy(&(pool->thcount_lock));
	pthread_cond_destroy(&(pool->threads_all_idle));
	free(pool->threads);
	free(pool);
}

int getNumofThreadWorking(struct threadpool* pool)
{
	return pool->num_working;
}

int create_thread(struct threadpool* pool, struct thread* pthread, int id)
{
	pthread = (struct thread*)malloc(sizeof(struct thread));
	if (pthread == NULL)
	{
		printf("create_thread():Could not allocate memory of thread\n");
		return -1;
	}
	pthread->pool = pool;
	pthread->id = id;
	pthread_create(&(pthread->pthread), NULL, (void*)thread_do, pthread);
	pthread_detach(pthread->pthread);
	return 0;
}

void* thread_do(struct thread* pthread)
{
	char thread_name[128] = {0};
	sprintf(thread_name, "thread_pool-%d", pthread->id);
	//prctl(PR_SET_NAME, thread_name);
	threadpool* pool = pthread->pool;
	pthread_mutex_lock(&(pool->thcount_lock));
	pool->num_threads++;
	pthread_mutex_unlock(&(pool->thcount_lock));
	while (pool->is_alive)
	{
		pthread_mutex_lock(&(pool->queue.has_jobs->mutex));
		pthread_cond_wait(&(pool->queue.has_jobs->cond), &(pool->queue.has_jobs->mutex));
		if (pool->queue.has_jobs->status == 0) continue;
		pthread_mutex_unlock(&(pool->queue.has_jobs->mutex));
		if (pool->is_alive)
		{
			pthread_mutex_lock(&(pool->thcount_lock));
			pool->num_working++;
			pthread_mutex_unlock(&(pool->thcount_lock));
			void (*func)(void*);
			void* arg;
			task* curtask = take_taskqueue(&(pool->queue));
			if (curtask)
			{
				func = curtask->function;
				arg = curtask->arg;
				func(arg);
				free(curtask);
			}
			pthread_mutex_lock(&(pool->thcount_lock));
			pool->num_working--;
			if (getNumofThreadWorking(pool) == 0)
				pthread_cond_broadcast(&(pool->threads_all_idle));
			pthread_mutex_unlock(&(pool->thcount_lock));
		}
	}
	pthread_mutex_lock(&(pool->thcount_lock));
	pool->num_threads--;
	pthread_mutex_unlock(&(pool->thcount_lock));
	return NULL;
}

void init_taskqueue(struct taskqueue* queue)
{
	queue->front = queue->rear = NULL;
	queue->len = 0;
	pthread_mutex_init(&(queue->mutex), NULL);
	queue->has_jobs = (staconv*)malloc(sizeof(struct staconv));
	queue->has_jobs->status = 0;
	pthread_mutex_init(&(queue->has_jobs->mutex), NULL);
	pthread_cond_init(&(queue->has_jobs->cond), NULL);
}

void destroy_taskqueue(struct taskqueue* queue)
{
	task *curtask, *next;
	curtask = queue->front;
	while((queue->len)--)
	{
		next = curtask->next;
		free(curtask);
		curtask = next;
	}
	queue->front = queue->rear = NULL;
	pthread_mutex_destroy(&(queue->mutex));
	pthread_mutex_destroy(&(queue->has_jobs->mutex));
	pthread_cond_destroy(&(queue->has_jobs->cond));
	free(queue->has_jobs);
}

void push_taskqueue(struct taskqueue* queue, struct task* curtask)
{
	pthread_mutex_lock(&(queue->mutex));
	if (queue->len == 0)
		queue->front = queue->rear = curtask;
	else
	{
		queue->rear->next = curtask;
		queue->rear = curtask;
	}
	queue->len++;
	pthread_mutex_lock(&(queue->has_jobs->mutex));
	queue->has_jobs->status = 1;
	pthread_cond_signal(&(queue->has_jobs->cond));
	pthread_mutex_unlock(&(queue->has_jobs->mutex));
	pthread_mutex_unlock(&(queue->mutex));
}

struct task* take_taskqueue(struct taskqueue* queue)
{
	task *first;
	pthread_mutex_lock(&(queue->mutex));
	first = queue->front;
	if (queue->len == 1)
	{
		queue->front = queue->rear = NULL;
		pthread_mutex_lock(&(queue->has_jobs->mutex));
		queue->has_jobs->status = 0;
		pthread_mutex_unlock(&(queue->has_jobs->mutex));
	}
	else
		queue->front = queue->front->next;
	queue->len--;
	pthread_mutex_unlock(&(queue->mutex));
	return first;
}