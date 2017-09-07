
//TODO
/**
 * 	1. new socked
 * 	2. new listen
 * 	3. bind port
 * 	4. epoll wait
 * 	5. epoll_wait>0 then read
 * 	6. throw to php
 * 	7.
 */

//#include <sys/event.h>
#include "ty_server.h"

//设置非阻塞描述符
int setnonblocking( int fd )
{
    int old_option = fcntl( fd, F_GETFL );
    int new_option = old_option | O_NONBLOCK;
    fcntl( fd, F_SETFL, new_option );
    return old_option;
}


char response[MAXLENGTH];
int resLength;

//tyWorker变量保存worker进程信息
tyWorker  workers[WORKER_NUM];
tyWorker worker;
//tyReactor 保存reactor线程信息
tyReactor  reactors[REACTOR_NUM];


//通过连接fd，获取主进程管道(后续用于获取reactor线程管道)
int connFd2WorkerId[1000];

//保存本进程信息


struct epoll_event ev,events[20];//ev用于注册事件,数组用于回传要处理的事件
int i, maxi, listenfd, new_fd, sockfd,epfd,nfds;

int setOutPut(char * data,int fd,int length){
	printf("setOutPut fd %d \n",fd);
	printf("epfd fd %d \n",epfd);

	resLength =length;

	memcpy(response, data, resLength);
	//strcpy(response, data);	//data 中包含 \0(可能) 不能使用strcpy
	ev.data.fd=fd;//设置用于写操作的文件描述符
	ev.events=EPOLLOUT|EPOLLET;//设置用于注测的写操作事件
	epoll_ctl(epfd,EPOLL_CTL_MOD,fd,&ev);//修改sockfd上要处理的事件为EPOLLOUT
}

int epollCreate(){
	int tmpEpFd;
    //生成用于处理accept的epoll专用的文件描述符
	tmpEpFd=epoll_create(512);
    return tmpEpFd;
}

int epollAdd(int epollfd,int readfd, int fdtype){
	struct epoll_event e;
	setnonblocking(readfd);
	//生成用于处理accept的epoll专用的文件描述符
//	epfd=epoll_create(256);
	//设置与要处理的事件相关的文件描述符
	e.data.fd=readfd;
	//设置要处理的事件类型
	e.events=fdtype;
//	e.events=SW_FD_PIPE | SW_EVENT_READ;
	//注册epoll事件
	epoll_ctl(epollfd,EPOLL_CTL_ADD,readfd,&e);

	return SW_OK;
}


int epollEventSet(int efd, int fd, int events) {
    struct epoll_event ev;
    memset(&ev, 0, sizeof(ev));
    ev.events = events;
    ev.data.fd = fd;

    int r = epoll_ctl(efd, EPOLL_CTL_MOD, fd, &ev);

    return SW_OK;
}

int masterSocks[2];


/**
 * ReactorThread main Loop
 */
void * swReactorThread_loop(int reactor_id)
{
//	int reactor_id = 0;
	int pipe_fd;
	int epollfd;
	struct epoll_event local_events[20];

	pthread_t thread_id = pthread_self();

	//创建epoll
	epollfd = epollCreate();

	printf("swReactorThread_loop reactor_id %d epollfd %d \n",reactor_id,epollfd);



	//创建epoll，监听某个worker进程的pipemasterfd
	for(i = 0; i < WORKER_NUM; i++){
		if (i % REACTOR_NUM == reactor_id)
		{
			pipe_fd = workers[i].pipMasterFd;

			//添加监听事件，监听管道
			int fdtype = EPOLLIN|EPOLLET;
			//TODO fdtype 是否需要转化 swReactorEpoll_event_set
			printf("reactor_id %d worker_id %d pipe_fd %d \n",reactor_id,i,pipe_fd);
			epollAdd(epollfd,pipe_fd,fdtype);


		}
	}
	//存储线程相关信息
	reactors[reactor_id].pidt = thread_id;
	reactors[reactor_id].epfd = epollfd;

	//后续主进程接受到连接时，会为某个reactor线程添加连接的监听事件
	 int nfds;
		int i;
		int pipefd;
		ssize_t n;
			char line[MAXLENGTH];
			int ret;
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

				//TODO 区分主进程抛过来的连接，还是worker进程写回的数据
				if(events[i].events&EPOLLIN && local_events[i].data.fd!=pipe_fd)//不是worker返回数据，则认为是主进程添加的连接监听
				{
					sockfd = local_events[i].data.fd;
					printf("connfd sockfd %d",sockfd);
						printf("read 0\n");
						if ( (n = recv(sockfd, line, MAXLENGTH, 0)) < 0){
								printf("read n %d \n",n);
							if (errno == ECONNRESET)
							{
								close(sockfd);
								local_events[i].data.fd = -1;
							}else{
								printf("readline error");
							}
						}else if (n == 0){
								printf("read error \n");
								close(sockfd);
								local_events[i].data.fd = -1;
						}
						printf("line %s \n",line);
						printf("line1 %c \n",line[20]);
						printf("line2 %zu \n",sizeof(line));
						printf("n %zu \n",n);

						//将接受到的请求抛给PHP
			//                	php_tinys_onReceive(sockfd,line,n);
							//TODO 写入管道,抛给worker子进程
						//取取余数获取worker进程，将数据写入其监听的管道中
						int i = sockfd%WORKER_NUM;
						printf(" rand worker id %d \n",i);
						int pipeWriteFd = workers[i].pipMasterFd;
						int ret;

						swEventData task;
						task.info.from_fd = sockfd;
						task.info.len = n;
						//将主进程的管道id传给worker进程[后续要传入reactor线程的管道id]
						task.info.topipe_fd = masterSocks[0];





						//TODO 后续多个reactor线程，需要记录线程id
						// 需要写一个结构体，传入连接fd
						memcpy(task.data, line, n);

						printf("write worker_pipe_fd fd %d \n",pipeWriteFd);

						//跑给worker进程后就不再监听本次连接
						epoll_ctl( epollfd, EPOLL_CTL_DEL, sockfd, 0 );

						ret = write(pipeWriteFd, &task, sizeof(task));
				}else if(local_events[i].events&EPOLLIN){	//接受worker进程返回的数据
					printf("master rec worker pipe \n");
					//TODO worker进程返回的数据，接收完发给客户端
					swEventData task;
					int n;

					if ((n=recv(local_events[i].data.fd, &task, sizeof(task), 0)) > 0)
					{
						//TODO 判断是否可以直接输出，还是必须修改epoll事件状态
						//修改事件状态为输出
//						setOutPut(task.data,task.info.from_fd,task.info.len);
						ret =  write(task.info.from_fd, task.data, task.info.len);
						printf("ret %d \n",ret);
						if (ret<0)
						{
							printf("errno %d \n",errno);
						}

						close( task.info.from_fd );
					}
				}
			}
		}
}

int runServer(char* ip,int port){
	int ret;
	//创建主进程管道 作废，都使用worker进程的管道
//	ret = socketpair(AF_UNIX, SOCK_DGRAM, 0, masterSocks);

	int pid =getpid();
	int forkPid ;
	int mainEpollFd;
	printf("pid  %d\n",pid);

	//创建manager进程及其下的各个worker子进程
	int workerNum ;
	workerNum = WORKER_NUM;
	forkPid = manageProccess(workerNum);
	if(forkPid==0){	//
//		exit(1);
		return 0;
	}

	//TODO 获取pid
	int pid1 =getpid();
	printf("pid1  %d\n",pid1);


	//TODO 第一步实现：主进程中直接取出数据，抛给worker进程
	mainEpollFd = mainReactorRun(ip, port);

	//TODO 主进程创建各个reactor线程，之后主进程循环监听listen事件accept后抛给reactor线程处理，
	printf("after mainReactorRun listenfd %d\n",listenfd);
	//创建reactor线程
	int reactorNum = 2;
	pthread_t pidt;
	for (i = 0; i < reactorNum; i++)
	{
		printf("pthread_create %d\n",i);
		if (pthread_create(&pidt, NULL, swReactorThread_loop, i) < 0)
		{
			printf("pthread_create[tcp_reactor] failed. Error: %s[%d]", strerror(errno), errno);
		}
		printf("pidt %d \n",pidt);
	}
	printf("after pthread_create listenfd %d \n",listenfd);
	//主进程开始循环监听
	mainReactorWait(mainEpollFd);


//	return server(ip, port);
}


//服务启动
int mainReactorRun(char* ip,int port)
{
	printf("mainReactorRun ip %s port %d \n",ip,port);

    ssize_t n;
    char line[MAXLENGTH];
    socklen_t clilen;
    //struct epoll_event ev,events[20];//ev用于注册事件,数组用于回传要处理的事件
    struct sockaddr_in clientaddr, serveraddr;
    //生成socket文件描述符
    listenfd = socket(AF_INET, SOCK_STREAM, 0);
    //把socket设置为非阻塞方式
    setnonblocking(listenfd);
    //生成用于处理accept的epoll专用的文件描述符
//    epfd=epoll_create(256);
    epfd = epollCreate();
    //添加监听事件，监听端口
    int fdtype =EPOLLIN|EPOLLET;
    epollAdd(epfd,listenfd,fdtype);

    //设置服务器端地址信息
    bzero(&serveraddr, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    //监听的地址
    char *local_addr= ip;
    inet_aton(local_addr,&(serveraddr.sin_addr));
    serveraddr.sin_port=htons(port);
    //绑定socket连接
    bind(listenfd, ( struct sockaddr* )&serveraddr, sizeof(serveraddr));
    //监听
    listen(listenfd, LISTENQ);
    maxi = 0;
    printf("listenfd %d epfd %d \n",listenfd,epfd);



    //添加管道监听，接受从worker近处返回的数据，发送给客户端
    //创建管道，epoll监听
    int ret;

    int readfd,writefd;

    //获取用于读取的fd
//     readfd = masterSocks[1];
//     writefd = masterSocks[0];
//     printf("master pipe readfd %d writefd %d \n",readfd,writefd);
//     int pipe_fdtype = SW_FD_PIPE | SW_EVENT_READ;
//     int pipe_fdtype = EPOLLIN|EPOLLET;
	 //TODO fdtype 是否需要转化 swReactorEpoll_event_set
//	 epollAdd(epfd,readfd,pipe_fdtype);
//	 epollAdd(epfd,writefd,pipe_fdtype);

    return listenfd;
}

int mainReactorWait(int mainEpollFd){
	printf("after mainReactorWait mainEpollFd %d \n",mainEpollFd);

	int readfd,writefd;
	int nfds;
	socklen_t clilen;
	    //struct epoll_event ev,events[20];//ev用于注册事件,数组用于回传要处理的事件
	    struct sockaddr_in clientaddr;
	while(1)
	    {
			/* epoll_wait：等待epoll事件的发生，并将发生的sokct fd和事件类型放入到events数组中；
			* nfds：为发生的事件的个数。可用描述符数量
			* 注：
			*/
	        nfds=epoll_wait(epfd,events,20,500);
	        //处理可用描述符的事件
	        for(i=0;i<nfds;++i)
	        {

	    		if(events[i].events&EPOLLIN){
	    			//打印出所有in事件
	    			printf("events[i].data.fd %d \n",events[i].data.fd);
	    		}

	        		printf("nfds %d listenfd %d \n",nfds,mainEpollFd);
	        		//当监听端口描述符可用时，接收链接的时候

	            if(events[i].data.fd==mainEpollFd)
	            {
	            		/* 获取发生事件端口信息，存于clientaddr中；
	                *new_fd：返回的新的socket描述符，用它来对该事件进行recv/send操作*/
	            	printf("accept\n");
	                new_fd = accept(mainEpollFd,(struct sockaddr *)&clientaddr, &clilen);
	                printf("new_fd %d \n",new_fd);
	                if(new_fd<0)
				   {
	                    perror("new_fd<0\n");
	                    perror("test\n");
	                    return 1;
	                }
	                perror("setnonblocking\n");
//	                setnonblocking(new_fd);
	                char *str = inet_ntoa(clientaddr.sin_addr);

	                //给reactor线程添加监听事件，监听本次连接
	                int reactor_id = new_fd%REACTOR_NUM; //连接fd对REACTOR_NUM取余，决定跑给哪个reactor线程
	                int reactor_epfd = reactors[reactor_id].epfd;
	                printf("reactor_epfd %d new_fd %d \n",reactor_epfd,new_fd);
				   int fdtype =EPOLLIN|EPOLLET;
				   epollAdd(reactor_epfd,new_fd,fdtype);

	            }


	        }
	    }
}

/**
 * 将php返回的数据，写入主进程的管道中，由主进程发送给客户端
 */
int send2ReactorPipe(char * data,int fd,int length){
	int ret;
	swEventData task;
	task.info.from_fd = fd;
	task.info.len = length;
	memcpy(task.data, data, length);

	//worker进程取不到fork之后在主进程创建的变量
	int masterWritePipe = worker.pipWorkerFd;



	printf("send2ReactorPipe fd %d \n",fd);
	printf("send2ReactorPipe masterWritePipe %d \n",masterWritePipe);
	ret = write(masterWritePipe, &task, sizeof(task));

	return 1;
}
