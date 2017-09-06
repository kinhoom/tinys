#include <sys/socket.h>

#include "ty_server.h"



//引用worker各个子进程的的信息变量
extern tyWorker workers[WORKER_NUM];
/**
 * 创建manager进程
 */
int manageProccess(int workerNum)
{
    pid_t pid;
    int i;

	pid = fork();
	switch (pid)
	{
	//创建manager进程
	case 0:
		for (i = 0; i < workerNum; i++)
		{
			//close(worker_pipes[i].pipes[0]);
			pid = tyManager_spawn_worker( i);
			if (pid < 0)
			{
				printf("fork() failed.");
				return SW_ERR;
			}
			else
			{
				//TODO 全局变量保存各个子进程的pid
				workers[i].pid = pid;
			}
		}
	}
	return 1;
}

int tyWorker_loop(int worker_id){
	int ret;
	int socks[2];
	int pipefd;
	int readfd;
	struct epoll_event local_events[20];


	int epollfd;
	//TODO 监听pipe管道事件

	//创建管道，epoll监听
    ret = socketpair(AF_UNIX, SOCK_DGRAM, 0, socks);

    //获取用于读取的fd
    readfd = socks[1];
    workers[worker_id].pipReadFd = readfd;
    workers[worker_id].pipWriteFd = socks[0];

    //创建epoll
    epollfd = epollCreate();
    //添加监听事件，监听管道
//    int fdtype = SW_FD_PIPE | SW_EVENT_READ;
    int fdtype = EPOLLIN|EPOLLET;

    //TODO fdtype 是否需要转化 swReactorEpoll_event_set
    epollAdd(epollfd,readfd,fdtype);

    int nfds;
    int i;
    //循环等待事件触发
    while(1)
	{
		/* epoll_wait：等待epoll事件的发生，并将发生的sokct fd和事件类型放入到events数组中；
		* nfds：为发生的事件的个数。可用描述符数量
		* 注：
		*/
		nfds=epoll_wait(epollfd,local_events,20,500);
		//处理可用描述符的事件
		for(i=0;i<nfds;++i)
		{
				printf("nfds %d \n",nfds);
				//当监听端口描述符可用时，接收链接的时候
				if(local_events[i].events&EPOLLIN)//当数据进入触发下面的流程
				{
					pipefd = local_events[i].data.fd;
					//从pipe中读取数据
					swWorker_onPipeReceive(pipefd);

				}
		}
	}
}

/**
 * 从管道中读取数据，并发送给php
 */
int swWorker_onPipeReceive(int fd){
    swEventData task;
//    char line[MAXLENGTH];
    int n;

	if ((n=recv(fd, &task, sizeof(task), 0)) > 0)
	{
		//需要将连接fd传给php
		php_tinys_onReceive(task.info.from_fd,task.data,task.info.len);

	}
}


//static
 pid_t tyManager_spawn_worker(int worker_id)
{
    pid_t pid;
    int ret;

    pid = fork();

    //fork() failed
    if (pid < 0)
    {
        swWarn("Fork Worker failed. Error: %s [%d]", strerror(errno), errno);
        return SW_ERR;
    }
    //worker child processor
    else if (pid == 0)
    {
    	printf("worker child processor \n");
    	//TODO 监听pipe管道事件

        ret = tyWorker_loop( worker_id);
        exit(ret);
    }
    //parent,add to writer
    else
    {
    	printf("worker child pid %d \n",pid);

        return pid;
    }
}


