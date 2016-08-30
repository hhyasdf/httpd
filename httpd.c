#include<stdio.h>
#include<sys/socket.h>
#include<errno.h>
#include<strings.h>
#include<signal.h>
#include<pthread.h>
#include<string.h>
#include<netinet/in.h>
#include<unistd.h>
#include<stdlib.h>
#include<setjmp.h>
#include<fcntl.h>
#include<sys/stat.h>
#include<arpa/inet.h>
#include<sys/wait.h>
#include<syslog.h>
#include<sys/resource.h>
#include<sys/mman.h>
#include<dirent.h>

#define MAXLINE 1024
#define MAXPATH 255
#define MAXVALUE 2048

#define GETMETHOD 1
#define POSTMETHOD 2

typedef struct{
    pthread_mutex_t file_buff_lock;
    void *addr;
    char *filename;
    int count;
    unsigned long size;
}FileBuff;

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


static char root_url[MAXPATH];
static int port;
static int thread_num;
FileBuff *file_buff;

void err_print(char *str);
int import_conf();
int build_listen_socket();
int read_str(int fd, char *buff);
int read_len(int fd, char *buff, int len);
int not_found(int connfd);
int notsupport_HTTPversion(int connfd);
int notsuport_method(int connfd);
int bad_request(int connfd);
int cannot_exec(int connfd);
int read_method(int connfd);
int get_url(int connfd, char *url, char *value);
int file_type(char *url, const char *type);
int exec_CGI(int connfd, char *path, int method, char *value);
int send_file(int connfd, char *url);
int GET_handle(int connfd);
int POST_handle(int connfd);
int accept_request(int listen_socket);
int change_url(char *url);
int lockfile(int lockfd);
int unlockfile(int lockfd);
int server_init(char *name, char *opt);
void *work_pthread(void *arg);
Thread_pool *pool_init(int thread_num);
int add_work(Thread_pool *pool, void *(*work_fun)(int), int arg);

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

//错误输出

void err_print(char *str)
{
    perror(str);
    exit(1);
}

//创建和导入配置文件

int import_conf()
{
    int conf;
    char name[] = "/etc/httpd_HTTPserver.conf";
    char buff[MAXPATH];

    umask(0);
    if(!access(name, F_OK))
    {
        conf = open(name, O_RDONLY);

        while(read_str(conf, buff) > 0)
        {
            if(!strcmp(buff, "listen_port:"))
            {
                read_str(conf, buff);
                port = atoi(buff);
            }
            else if(!strcmp(buff, "root_directory:"))
            {
                read_str(conf, buff);
                strcpy(root_url, buff);
            }
            else if(!strcmp(buff, "thread_number:"))
            {
                read_str(conf, buff);
                thread_num = atoi(buff);
            }
            else
            {
                printf("import error\n");
                exit(1);
            }
        }
    }
    else
    {
        conf = open(name, O_CREAT | O_WRONLY, S_IWUSR | S_IRUSR);

        printf("please enter listen port: ");
        scanf("%d", &port);
        printf("please enter root directory: ");
        scanf("%s", root_url);
        printf("please enter the number of threads: ");
        scanf("%d", &thread_num);
        if(thread_num > 500)
        {
            thread_num = 500;
        }

        dprintf(conf, "listen_port: %d\n", port);
        dprintf(conf, "root_directory: %s\n", root_url);
        dprintf(conf, "thread_number: %d\n", thread_num);
    }

    if(opendir(root_url) == NULL)
    {
        printf("root directory does not exist\n");
        unlink(name);
        exit(0);
    }

    close(conf);
    return 0;
}

//建立监听套接字

int build_listen_socket()
{
    int lstfd;
    struct sockaddr_in lstaddr;

    bzero(&lstaddr, sizeof(lstaddr));
    lstaddr.sin_family = AF_INET;
    lstaddr.sin_port = htons(port);
    lstaddr.sin_addr.s_addr = htonl(INADDR_ANY);

    if((lstfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        err_print("build error");
    }

    if(bind(lstfd, (struct sockaddr *)&lstaddr, sizeof(lstaddr)) < 0)
    {
        err_print("bind error");
    }

    if(listen(lstfd, 10) < 0)
    {
        err_print("listen error");
    }

    return lstfd;
}

//读写套接字函数

int read_str(int fd, char *buff)
{
    int readlen = 0;
    char *p = buff;

    while(read(fd, p, 1) > 0 && *p != ' ' && *p != '\n')
    {
        readlen ++;
        p ++;
    }
    *p = '\0';
    return readlen;
}


int read_len(int fd, char *buff, int len)
{
    int readlen = 0;
    char *p = buff;

    while(read(fd, p, 1) > 0 && readlen < len)
    {
        readlen ++;
        p ++;
    }
    *p = '\0';
    return readlen;
}


//输出找不到页面

int not_found(int connfd)
{
    int len;
    char not_found_page[] = "<h> 404 not found </h>";
    len = strlen(not_found_page);
    dprintf(connfd, "%s", "HTTP/1.1 404 Not Found\r\nContent-Type: text/html\r\nContent-Length: ");
    dprintf(connfd, "%d", len);
    dprintf(connfd, "\r\nConnection: close\r\n\r\n");
    dprintf(connfd, "%s", not_found_page);
    return 0;
}

//检查http协议版本

int notsupport_HTTPversion(int connfd)
{
    int len;
    char http_str[10];
    char notsuport[] = "<h>505 http version not supported</h>";
    len = strlen(notsuport);

    read_str(connfd, http_str);

    if(strcmp(http_str, "HTTP/1.1\r"))
    {
        dprintf(connfd, "HTTP/1.1 505 HTTP Version Not Supported\r\nContent-Type: text/html\r\nContent-Length: ");
        dprintf(connfd, "%d", len);
        dprintf(connfd, "\r\nConnection: close\r\n\r\n");
        dprintf(connfd, "%s", notsuport);
        return 1;
    }
    return 0;
}

//不支持除了POST和GET之外的方法

int notsuport_method(int connfd)
{
    int len;
    char notsuport[] = "<h>httpd donnot support the HTTP method</h>";
    len = strlen(notsuport);

    dprintf(connfd, "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nContent-Length: ");
    dprintf(connfd, "%d", len);
    dprintf(connfd, "\r\nConnection: close\r\n\r\n");
    dprintf(connfd, "%s", notsuport);
    return 0;
}

//400 bad request

int bad_request(int connfd)
{
    int len;
    char br[] = "<h>400 bad request</h>";
    len = strlen(br);

    dprintf(connfd, "HTTP/1.1 400 Bad Request\r\nContent-Type: text/html\r\nContent-Length: ");
    dprintf(connfd, "%d", len);
    dprintf(connfd, "\r\nConnection: close\r\n\r\n");
    dprintf(connfd, "%s", br);
    return 0;
}

//不能执行程序

int cannot_exec(int connfd)
{
    int len;
    char cannot_ex[] = "<h>500 Internal Server Error</h>";
    len = strlen(cannot_ex);

    dprintf(connfd, "HTTP/1.1 500 Internal Server Error\r\nContent-Type: text/html\r\nContent-Length: ");
    dprintf(connfd, "%d", len);
    dprintf(connfd, "\r\nConnection: close\r\n\r\n");
    dprintf(connfd, "%s", cannot_ex);
    return 0;
}

//读取报文方法

int read_method(int connfd)
{
    char meth[6];

    read_str(connfd, meth);
    if(!strcasecmp(meth, "GET"))
    {
        return GETMETHOD;
    }
    else if(!strcasecmp(meth, "POST"))
    {
        return POSTMETHOD;
    }
    else
    {
        return notsuport_method(connfd);
    }
}

//把url里的转码后的空格改回来

int change_url(char *url)
{
    char *p1, *p2, temp[MAXPATH];
    strcpy(temp, url);

    bzero(url, MAXPATH);
    for(p1 = temp, p2 = url; *p1 != '\0'; p1 ++, p2 ++)
    {
        if(*p1 == '%')
        {
//            *p2++ = '\\';
            *p2 = ' ';
            p2 ++;
            p1 += 3;
        }
        *p2 = *p1;
    }

    p2 = '\0';
    return 0;
}

//获取url拼凑成绝对路径
//分离url后带的数据

int get_url(int connfd, char *url, char *value)
{
    char *p;
    int change_flag = 0;
    p = url + strlen(root_url);

    strcpy(url, root_url);
    read_str(connfd, p);

    if(!strcmp(p, "/"))
    {
        strcat(url, "index.html");
    }

    if(value != NULL)
    {
        for(; *p != '?' && *p != '\0'; p ++)
        {
            if(*p == '%')
            {
                change_flag = 1;
            }
        }
        if(*p == '?')
        {
            *p = '\0';
            p ++;
            strcpy(value, p);
        }
        else
        {
            *value = '#';
        }
    }

    if(change_flag == 1)
    {
        change_url(url);
    }
//    printf("%s\n%s\n", url, value);
    return 0;
}

//执行CGI程序

int exec_CGI(int connfd, char *path, int method, char *value)
{
    int cgi_input[2];
    int cgi_output[2];
    char temp_buff[MAXPATH], c;
    int length, i = 0, n;
    pid_t pid;

    if(pipe(cgi_input) < 0)
    {
        cannot_exec(connfd);
        return 0;
    }
    if(pipe(cgi_output) < 0)
    {
        cannot_exec(connfd);
        return 0;
    }

    if(method == POSTMETHOD)
    {
        while(strcmp(temp_buff, "Content-Length:") && strcmp(temp_buff, "\r"))
        {
            read_str(connfd, temp_buff);
        }
        if(!strcmp(temp_buff, "\r"))
        {
            bad_request(connfd);
            return 0;
        }
        else
        {
            read_str(connfd, temp_buff);
            length = atoi(temp_buff);
            while(strcmp(temp_buff, "\r"))
            {
                read_str(connfd, temp_buff);
            }
        }
    }

    if((pid = fork()) < 0)
    {
        cannot_exec(connfd);
    }
    else if(pid == 0)      //chile
    {
        char env_buff[MAXVALUE + 20];

        close(cgi_input[1]);
        close(cgi_output[0]);
        dup2(cgi_input[0], 0);
        dup2(cgi_output[1], 1);

        if(method == GETMETHOD)
        {
            putenv("REQUEST_METHOD=GET");
            sprintf(env_buff, "QUERY_STRING=%s", value);
            putenv(env_buff);
        }
        else
        {
            putenv("REQUEST_METHOD=POST");
            sprintf(env_buff, "CONTENT_LENGTH=%d", length);
            putenv(env_buff);
        }

        execl(path, path, NULL);
        exit(0);
    }
    else      //parent
    {
        close(cgi_input[0]);
        close(cgi_output[1]);
        dup2(cgi_input[1], 1);
        dup2(cgi_output[0], 0);

        if(method == POSTMETHOD)
        {
            while(i < length)
            {
                read(connfd, &c, 1);
                write(cgi_input[1], &c, 1);
                i ++;
            }
        }

        while((n = read(cgi_output[0], temp_buff, MAXPATH)) > 0)
        {
            write(connfd, temp_buff, n);
        }

        close(cgi_input[1]);
        close(cgi_output[0]);
        waitpid(pid, NULL, 0);
    }

    return 0;
}

//发送文件内容

int send_file(int connfd, char *url)
{
    int sourcefile, n;
    long unsigned int file_size;
    char type_buff[MAXPATH], write_buff[MAXLINE];
    char *type = url;
    struct stat statbuff;
    bzero(&statbuff, sizeof(statbuff));

    sourcefile = open(url, O_RDONLY);

    fstat(sourcefile, &statbuff);
    file_size = statbuff.st_size;

    for(n = 0; n < strlen(url); n ++)
    {
        if(url[n] == '.')
        {
            type = url + n;
        }
    }
    if(*type != '.')
    {
        return -1;
    }
    type ++;

    if(!strcmp(type, "css"))
    {
        sprintf(type_buff, "text/css");
    }
    else if(!strcmp(type, "html"))
    {
        sprintf(type_buff, "text/html");
    }
    else if(!strcmp(type, "js"))
    {
        sprintf(type_buff, "application/x-javascript");
    }
    else if(!strcmp(type, "jpeg"))
    {
        sprintf(type_buff, "iamge/jpeg");
    }
    else if(!strcmp(type, "jpg"))
    {
        sprintf(type_buff, "image/jpeg");
    }
    else if(!strcmp(type, "png"))
    {
        sprintf(type_buff, "iamge/png");
    }
    else if(!strcmp(type, "gif"))
    {
        sprintf(type_buff, "iamge/gif");
    }
    else if(!strcmp(type, "cgi"))
    {
        return -1;
    }
    else
    {
        not_found(connfd);
        return 0;
    }

    if(pthread_mutex_lock(&(file_buff->file_buff_lock)) == 0);
    {
        if(file_buff->addr == NULL || file_buff->count == 10)
        {
            if(file_buff->addr != NULL)
            {
                munmap(file_buff->addr, file_buff->size);
                free(file_buff->filename);
                file_buff->addr = NULL;
                file_buff->filename = NULL;
            }
            file_buff->addr = mmap(0, file_size, PROT_READ, MAP_PRIVATE, sourcefile, 0);
            file_buff->size = file_size;
            file_buff->count = 0;
            file_buff->filename = (char *)malloc(strlen(url) * sizeof(char));
            strcpy(file_buff->filename, url);
        }

        if(!strcmp(url, file_buff->filename))
        {
            unsigned long int off = 0;

            dprintf(connfd, "HTTP/1.1 200 OK\r\n");
            dprintf(connfd, "Content-Type: %s\r\n", type_buff);
            dprintf(connfd, "Content-Length: %ld\r\n", file_size);
            dprintf(connfd, "Connection: close\r\n\r\n");
            while(off < file_buff->size)
            {
                off += write(connfd, file_buff->addr + off, MAXLINE);
            }

            close(sourcefile);
            pthread_mutex_unlock(&(file_buff->file_buff_lock));
            return 0;
        }
        pthread_mutex_unlock(&(file_buff->file_buff_lock));
    }

    dprintf(connfd, "HTTP/1.1 200 OK\r\n");
    dprintf(connfd, "Content-Type: %s\r\n", type_buff);
    dprintf(connfd, "Content-Length: %ld\r\n", file_size);
    dprintf(connfd, "Connection: close\r\n\r\n");
    while((n = read(sourcefile, write_buff, MAXLINE)) > 0)
    {
        write(connfd, write_buff, MAXLINE);
    }
    close(sourcefile);
    return 0;
}

//GET处理函数
//如果是CGI程序即执行

int GET_handle(int connfd)
{
    char value[MAXPATH], url[MAXPATH];

    get_url(connfd, url, value);

    if(access(url, F_OK) != 0)
    {
        not_found(connfd);
        return 0;
    }

    if(send_file(connfd, url) < 0)
    {
        exec_CGI(connfd, url, GETMETHOD, value);
    }

    return 0;
}

//POST处理函数
//执行CGI程序

int POST_handle(int connfd)
{
    char url[MAXPATH];

    get_url(connfd, url, NULL);

    if(access(url, F_OK) != 0)
    {
        not_found(connfd);
        return 0;
    }

    exec_CGI(connfd, url, POSTMETHOD, NULL);

    return 0;
}

//服务器守护初始化
//返回监听套接字

int lockfile(int lockfd)
{
    struct flock lock;

    lock.l_type = F_WRLCK;
    lock.l_whence = SEEK_SET;
    lock.l_start = 0;
    lock.l_len = 0;

    return fcntl(lockfd, F_SETLK, &lock);
}

int unlockfile(int lockfd)
{
    struct flock lock;

    lock.l_type = F_UNLCK;
    lock.l_whence = SEEK_SET;
    lock.l_start = 0;
    lock.l_len = 0;

    return fcntl(lockfd, F_SETLK, &lock);
}


int server_init(char *name, char *opt)
{
    int fd0, fd1, fd2, i = 0, lstfd = -1, lockfd;
    char p_name[50];
    pid_t pid;
    struct rlimit rl;

    if(opt == NULL || strcmp(opt, "-k"))
    {

        import_conf();

        file_buff = (FileBuff *)malloc(sizeof(FileBuff));
        file_buff->filename = NULL;
        file_buff->addr = NULL;
        file_buff->count = -1;
        file_buff->size = 0;
        pthread_mutex_init(&(file_buff->file_buff_lock), NULL);

        printf("start working\nlistening port: %d\nroot directory: %s\nthread number: %d\n", port, root_url, thread_num);

        if(getrlimit(RLIMIT_NOFILE, &rl) < 0)
        {
            err_print("cannot get file limit");
        }

        if((pid = fork()) < 0)
        {
            err_print("cannot build child process");
        }
        else if(pid != 0)
            exit(0);

        if(chdir("/") < 0)
        {
            err_print("cannot change working directory");
        }

        setsid();

        lstfd = build_listen_socket();


        if(rl.rlim_max == RLIM_INFINITY)
        {
            rl.rlim_max = 1024;
        }

        signal(SIGPIPE, SIG_IGN);
        signal(SIGHUP, SIG_IGN);

        for(; i < rl.rlim_max; i ++)
        {
            if(i != lstfd)
            close(i);
        }

        fd0 = open("/dev/null", O_RDWR);
        fd1 = dup(0);
        fd2 = dup(0);

        openlog(name, LOG_CONS, LOG_DAEMON);
        if(fd0 != 0 || fd1 != 1 || fd2 != 2)
        {
            syslog(LOG_ERR, "unexpect file descriptors %d %d %d", fd0, fd1, fd2);
            exit(1);
        }
    }

    lockfd = open("/var/run/httpd_HTTPserver.pid", O_CREAT | O_RDWR, S_IRUSR | S_IWUSR | S_IROTH);
    if(lockfd < 0)
    {
        syslog(LOG_ERR, "cannot open httpd_HTTPserver.pid");
        exit(1);
    }
    if(lockfile(lockfd) < 0)
    {
        syslog(LOG_ERR, "there already have a httpd procces");
        if(opt != NULL && !strcmp(opt, "-k"))
        {
            struct flock fl;

            fl.l_type = F_WRLCK;
            fl.l_whence = SEEK_SET;
            fl.l_start = 0;
            fl.l_len = 0;

            fcntl(lockfd, F_GETLK, &fl);

            kill(fl.l_pid, SIGKILL);
        }
        exit(1);
    }
    if(opt != NULL && !strcmp(opt, "-k"))
    {
        exit(1);
    }

    return lstfd;
}

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
