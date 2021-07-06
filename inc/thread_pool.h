#ifndef _THREAD_POOL_H
#define _THREAD_POOL_H
#include <stdlib.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#define MAX_WAITIN_TASK 1000
#define MAX_ACTIVE_THREAD 20


struct task//任务节点
{
	void*(*do_task)(void*);//函数指针，指向要执行的函数
	void*arg;//给执行函数的参数
	
	struct task*next;//指向下一个任务节点
};                        

struct thread_pool//线程池
{
	pthread_mutex_t mutex;//线程互斥锁，用于保护任务队列，有线程拿任务的时候先上锁，添加任务节点的时候也先上锁
	pthread_cond_t cond;//条件变量，没有任务节点时让线程等待，条件产生，唤醒线程
	bool shutdown;    //线程池的销毁标记
	
	pthread_t *tid;//指向线程ID的数组
	struct task*task_first;//指向第一个任务节点
	unsigned int max_waiting_tasks;//队列上最多可以有多少个任务节点在同时等待
	unsigned int waiting_task;//正在等待的任务节点数
	unsigned int active_threads;//活动线程数（创建的线程数）
	//....
};
void handler(void*arg);
/*
	routine:任务执行函数
*/
void*routine(void*arg);
bool init_pool(struct thread_pool*pool,int tnread_num);
bool add_task(struct thread_pool*pool,void*(*do_task)(void*),void*arg);
bool destroy_pool(struct thread_pool*pool);

#endif