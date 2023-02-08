/* 
    略微优化的epoll
    可以选择ET/LT
*/
#include<sys/types.h>
#include<sys/socket.h>
#include<unistd.h>
#include<assert.h>
#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<netinet/in.h>
#include<errno.h>
#include<arpa/inet.h>
#include<fcntl.h>
#include<sys/epoll.h>
#include<pthread.h>

#define MAX_EVENT_NUMBER 1024
#define BUFFER_SIZE 10
#define BOOL int
#define TRUE 1
#define FALSE 0

/* 将文件描述符设置成非阻塞的 */
int setnonblocking(int fd)
{
	int old_option = fcntl(fd, F_GETFL);
	int new_option = old_option | O_NONBLOCK;
	fcntl(fd, F_SETFL, new_option);
	return old_option;
}

/* 将文件描述符 fd 上的 EPOLLIN 注册到 epollfd 指示的 epoll 内核事件表中，参数 enable_et 指定是否对 fd 启用 ET 模式 */
void addfd(int epollfd, int fd, BOOL enable_et)
{
	struct epoll_event event;
	event.data.fd = fd;
	event.events = EPOLLIN;
	if (enable_et)
	{
		event.events |= EPOLLET;
	}
	epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &event);
	setnonblocking(fd);
}

/* LT 模式工作流程 */
void lt(struct epoll_event* events, int number, int epollfd, int listenfd)
{
	char buf[BUFFER_SIZE];
	for (int i = 0; i < number; i++)
	{
		int sockfd = events[i].data.fd;
		if (sockfd == listenfd)
		{
			struct sockaddr_in client_address;
			socklen_t client_addrlength = sizeof(client_address);
			int connfd = accept(listenfd, (struct sockaddr*)&client_address, &client_addrlength);
			addfd(epollfd, connfd, FALSE);	/* 把 connfd 添加到内核事件表中，并禁用 ET 模式 */
		}
		else if (events[i].events & EPOLLIN)
		{
			/* 只要 socket 读缓存中还有未读出的数据，这段代码就被触发 */
			printf("EPOLLIN event trigger once\n");
			memset(buf, '\0', BUFFER_SIZE);
			int ret = recv(sockfd, buf, BUFFER_SIZE - 1, 0);
			if (ret <= 0)
			{
				close(sockfd);
				continue;
			}
			printf("get %d bytes of content: %s\n", ret, buf);
		}
		else
		{
			printf("something else happened \n");
		}
	}
}

/* ET 模式的工作流程 */
void et(struct epoll_event* events, int number, int epollfd, int listenfd)
{
	char buf[BUFFER_SIZE];
	for (int i = 0; i < number; i++)
	{
		int sockfd = events[i].data.fd;
		if (sockfd == listenfd)
		{
			struct sockaddr_in client_address;
			socklen_t client_addrlength = sizeof(client_address);
			int connfd = accept(sockfd, (struct sockaddr*)&client_address, &client_addrlength);
			addfd(epollfd, connfd, TRUE);	/* 把 connfd 添加到内核事件表中，并开启 ET 模式 */
		}
		else if (events[i].events & EPOLLIN)
		{
			/* 这段代码不会被重复触发，所以必须循环读取数据，以确保把 socket 读缓存中的所有数据读出 */
			printf("EPOLLIN event trigger once\n");
			while (1)
			{
				memset(buf, '\0', BUFFER_SIZE);
				int ret = recv(sockfd, buf, BUFFER_SIZE - 1, 0);
				if (ret < 0)
				{
					/* 对于非阻塞IO，下面的条件成立表示数据已全部读取完毕。此后，epoll就能再次触发 sockfd 上的 EPOLLIN 事件，进行下一次读操作*/
					if (errno == EAGAIN || errno == EWOULDBLOCK)
					{
						printf("raed later\n");
						break;
					}
					close(sockfd);
					break;
				}
				else if (ret == 0)	/* 返回0，表示对方已经关闭连接了 */
				{
					close(sockfd);
				}
				else 
				{
					printf("get %d bytes of content: %s\n", ret, buf);
				}
			}
		}
		else
		{
			printf("something else happened \n");
		}
	}
}

int main(int argc, char* argv[])
{
	if (argc <= 2)
	{
		return 1;
	}
	const char* ip = argv[1];
	int port = atoi(argv[2]);
	
	int ret = 0;
	struct sockaddr_in address;
	bzero(&address, sizeof(address));
	address.sin_family = AF_INET;
	address.sin_port = htons(port);
	inet_pton(AF_INET, ip, &address.sin_addr);

	int listenfd = socket(PF_INET, SOCK_STREAM, 0);
	assert(listenfd >= 0);

	ret = bind(listenfd, (struct sockaddr*)&address, sizeof(address));
	assert(ret != -1);

	ret = listen(listenfd, 5);
	assert(ret != -1);

	struct epoll_event events[MAX_EVENT_NUMBER];
	int epollfd = epoll_create(10);
	assert(epollfd != -1);
	addfd(epollfd, listenfd, TRUE);

	while (1)
	{
		int ret = epoll_wait(epollfd, events, MAX_EVENT_NUMBER, -1);
		if (ret < 0)
		{
			printf("epoll filure\n");
			break;
		}

		// lt(events, ret, epollfd, listenfd);
		et(events, ret, epollfd, listenfd);
	}

	close(listenfd);
	return 0;
}
