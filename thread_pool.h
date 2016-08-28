#include<pthread.h>
#include<stdlib.h>

//工作的数据类型
//包括执行的函数指针和参数

typedef struct Work{
    void *(*work_fun)(int arg) ;
    int arg;
    struct Work *next;
}Work;

//工作的队列

typedef struct{
    Work *head;
    Work *tail;
}Work_que;

//一个线程池结构
//包括锁、条件变量、线程池的线程数量、线程id数组、工作队列和数量

typedef struct{
    pthread_mutex_t work_que_lock;
    pthread_cond_t work_que_ready;

    int thread_num;
    pthread_t *thread_id;

    Work_que *work_que;
    int work_num;
}Thread_pool;


//线程的工作函数

void *work_pthread(void *arg)
{
    Thread_pool *pool = arg;
    Work *work;

    while(1)
    {
        pthread_mutex_lock(&(pool->work_que_lock));
        if(pool->work_num == 0)
        {
            pthread_cond_wait(&(pool->work_que_ready), &(pool->work_que_lock));
        }

        pool->work_num --;
        work = pool->work_que->head->next;
        pool->work_que->head->next = work->next;
        if(pool->work_que->head->next == NULL)
        {
            pool->work_que->tail = pool->work_que->head;
        }
        pthread_mutex_unlock(&(pool->work_que_lock));

        (*(work->work_fun))(work->arg);
        free(work);
        work = NULL;
    }
}

//初始化并创建一个线程池，返回线程池的指针
//线程默认分离

Thread_pool *pool_init(int thread_num)
{
    Thread_pool *pool = (Thread_pool *)malloc(sizeof(Thread_pool));
    pthread_attr_t attr;

    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

    pthread_mutex_init(&(pool->work_que_lock), NULL);
    pthread_cond_init(&(pool->work_que_ready), NULL);

    pool->thread_num = thread_num;
    pool->thread_id = (pthread_t *)malloc(thread_num * sizeof(pthread_t));

    pool->work_num = 0;
    pool->work_que = (Work_que *)malloc(sizeof(Work_que));

    pool->work_que->head = (Work *)malloc(sizeof(Work));
    pool->work_que->tail = pool->work_que->head;
    pool->work_que->head->work_fun = NULL;
    pool->work_que->head->arg = 0;
    pool->work_que->head->next = NULL;

    for(int i = 0; i < thread_num; i ++)
    {
        pthread_create(&(pool->thread_id[i]), &attr, work_pthread, pool);
    }

    return pool;
}

//向队列里增加一项工作

int add_work(Thread_pool *pool, void *(*work_fun)(int), int arg)
{
    Work *work = (Work *)malloc(sizeof(Work));
    work->work_fun = work_fun;
    work->arg = arg;
    work->next = NULL;

    pthread_mutex_lock(&(pool->work_que_lock));
    pool->work_num ++;
    pool->work_que->tail->next = work;
    pool->work_que->tail = work;
    pthread_mutex_unlock(&(pool->work_que_lock));

    pthread_cond_signal(&(pool->work_que_ready));

    return 0;
}

