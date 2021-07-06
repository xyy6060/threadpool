#include "thread_pool.h"

void*fun(void*arg)
{
	while(1)
	{
		printf("arg = %d\n",*(int*)arg);
		sleep(1);
	}
}
int main()
{
	
	struct thread_pool pool;//创建一个线程池，大小为sizeof(struct thread_pool)
	bool ok = init_pool(&pool,3);//初始化这个线程池，活动线程数量为3
	
	int a[5] = {1,2,3,4,5};//
	int i = 0;
	for(;i < 5;i++)
	{
		add_task(&pool,fun,(void*)&a[i]);//添加任务节点
	}
	
	destroy_pool(&pool);
}