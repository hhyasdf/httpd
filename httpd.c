#include"httpd.h"
#include"thread_pool.h"

void *GET_d(int connfd)
{
    GET_handle(connfd);
    sleep(3);
    close(connfd);
}

void *POST_d(int connfd)
{
    POST_handle(connfd);
    sleep(3);
    close(connfd);
}


int main(int argc, char **argv)
{
    int connfd, lstfd, method;
    struct sockaddr_in cliaddr;
    socklen_t clilen = sizeof(cliaddr);

    if(argc == 1)
    {
        lstfd = server_init(argv[0], NULL);
    }
    else
    {
        lstfd = server_init(argv[0], argv[1]);
    }

    Thread_pool *pool = pool_init(thread_num);

    while(1)
    {
        connfd = accept(lstfd, (struct sockaddr *)&cliaddr, &clilen);

        method = read_method(connfd);
        if(method == GETMETHOD)
        {
            add_work(pool, GET_d, connfd);
        }
        else if(method == POSTMETHOD)
        {
            add_work(pool, POST_d, connfd);
        }
    }

    close(lstfd);
    exit(0);
}
