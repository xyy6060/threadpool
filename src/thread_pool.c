#include "thread_pool.h"

/*
	清理函数
*/
void handler(void*arg)
{
	pthread_mutex_unlock((pthread_mutex_t*)arg);//解锁
}
/*
	routine:任务执行函数(线程函数)
*/
void*routine(void*arg)//此函数的固定类型：void*(void*)
{
	struct thread_pool*pool = (struct thread_pool*) arg;//将传来的参数强转为(struct thread_pool*)类型，并赋值给pool
	struct task*p = NULL;
	
	while(1)//永远不退出，可以重复执行任务
	{
		//访问任务队列前先上锁，为防止带锁退出，注册处理例程
		pthread_cleanup_push(handler,(void*)&pool->mutex);
		pthread_mutex_lock(&pool->mutex);
		
		//===============================
		//线程池没有销毁并且任务队列为空，则进入休眠等待条件变量
		if(!pool->shutdown && pool->waiting_task == 0)
		{
			pthread_cond_wait(&pool->cond,&pool->mutex);
								//唤醒的地方
								//1:在添加任务时候
								//2:销毁线程池的时候
		}
		
		//线程池销毁了并且等待的任务队列为空
		if(pool->shutdown && pool->waiting_task == 0)
		{
			pthread_mutex_unlock(&pool->mutex);//访问前上锁了，所以这里要解锁
			pthread_exit(NULL);//退出子线程
			
		}
		
		//当前有任务，拿任务
		p = pool->task_first;
		pool->task_first = p->next;
		p->next = NULL;
		pool->waiting_task--;
		
		//拿到任务后解锁，并出栈handler(但是不执行)
		pthread_mutex_unlock(&pool->mutex);
		pthread_cleanup_pop(0);
		
		//执行任务，并且在此期间禁止响应取消请求
		pthread_setcancelstate(PTHREAD_CANCEL_DISABLE,NULL);
		(p->do_task)(p->arg);//执行任务
		pthread_setcancelstate(PTHREAD_CANCEL_ENABLE,NULL);
		
		free(p);//释放p所指向的那块用mallo开辟的空间(pool->task_first执行完了就不需要了)，p还是存在的
		
	}
}
/*
	init_pool:初始化一个线程池
	@pool：要初始化的线程池
	@tnread_num：线程池中最先有多少条线程
*/
bool init_pool(struct thread_pool*pool,int thread_num)//需要传入一个线程池的地址和活动线程数
{
	pthread_mutex_init(&pool->mutex,NULL);//初始化线程互斥锁
	pthread_cond_init(&pool->cond,NULL);//初始化条件变量
	
	pool->shutdown = false;//线程销毁开关关闭
	pool->tid = malloc(sizeof(pthread_t)*MAX_ACTIVE_THREAD);//（线程ID数组开辟空间）此变量要在不同线程里用到所以要malloc开辟，每个线程ID的大小*总的线程数
	
	pool->task_first = NULL;//开始时还没有添加任务
	
	pool->max_waiting_tasks = MAX_WAITIN_TASK;//最大同时等待的任务量
	pool->waiting_task = 0;//最开始任务等待量为0
	pool->active_threads = thread_num;//活动线程数
	
	
	int i;
	for(i = 0;i < thread_num;i++)//创建thread_num个活动线程
	{														//第三个参数？？
		int r = pthread_create(&(pool->tid)[i],NULL,routine,(void*)pool);//第一个参数类型为地址，pool->tid的类型为pthread_t *是个地址类型，而数组名的类型也是地址类型，所以类型匹配，
		if(r == -1)
		{
			perror("create thread error:");//打印出错码
			printf("i = %d\n",i);//看创建第几个线程失败
			return false;//返回值为bool类型
		}
	}
	
	return true;
	
}

/*
	bool add_task:往线程池中投放任务
			
*/
bool add_task(struct thread_pool*pool,void*(*do_task)(void*),void*arg)//任务节点要执行的函数，任务节点包含三部分内容
{
	
	struct task*new_task = malloc(sizeof(*new_task));//创建任务节点
	//给结构体里的成员变量赋初值
	new_task->do_task = do_task;
	new_task->arg = arg;
	new_task->next = NULL;
	
	
	pthread_mutex_lock(&pool->mutex);//添加任务节点前先上锁
	
	
	if(pool->waiting_task == MAX_WAITIN_TASK)//如果任务队列满了则不添加节点，解锁，释放空间
	{
		pthread_mutex_unlock(&pool->mutex);
		free(new_task);
		return false;
	}
	//任务队列没满则开始添加节点
	struct task*p = pool->task_first;//遍历指针
	if(p == NULL)//任务队列为空
	{
		pool->task_first = new_task;//新创的节点为首节点
		
	}
	else//任务队列不为空
	{
		while(p->next != NULL)//找到最后一个节点
		{
			p = p->next;
		}
		p->next = new_task;//尾插法
		
	}
	pool->waiting_task++;//等待的任务数加一
	pthread_mutex_unlock(&pool->mutex);//任务节点添加完后解锁
	pthread_cond_signal(&pool->cond);//有任务了，马上唤醒睡眠的线程来执行任务
	
	return true;
}

/*
	destroy_pool:销毁线程池
			
*/
bool destroy_pool(struct thread_pool*pool)
{
	
	
	pool->shutdown = true;
	
	pthread_cond_broadcast(&pool->cond);//唤醒所有睡眠的线程
	//等待线程退出
	int i;
	for(i = 0;i < pool->active_threads;i++)
	{
		pthread_join(pool->tid[i],NULL);
	}
	
	//free memories
	free(pool->tid);
	
	//1.mutex  cond
	pthread_mutex_destroy(&pool->mutex);
	pthread_cond_destroy(&pool->cond);
}