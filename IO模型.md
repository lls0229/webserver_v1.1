# Unix/Linux 上的五种IO模型
## 阻塞 blocking
- 调用者调用了某个函数，等待这个函数返回，期间什么也做不了，不停的去检查这个函数有没有返回，必须等这个函数返回才能进行下一步动作

## 非阻塞 non-blokcing(NIO)
- 非阻塞等待，每隔一段时间就去检测IO事件是否就绪，没有就绪就可以做其他的事，非阻塞IO执行系统调用总是立即返回，不管事件是否已经发生，若事件没有发生，则返回-1，此时可以根据errno区分这两种情况，对于accept，recv，send，事件未发生时，errno通常被设置成EAGAIN。

## IO复用(IO multiplexing)
- Linux 用select/poll/epoll/ 函数实现IO 复用模型，这些函数也会使进程阻塞，但是和阻塞IO所不同的是这些函数可以同时阻塞多个IO操作。而且可以同时对多个读操作、写操作的IO函数进行检测，直到有数据可读或可写时，才真正调用IO操作函数。

## 信号驱动(signal-driven)
- Linux 用套接口进行信号驱动IO，安装一个信号处理函数，进程继续运行并不阻塞，当IO事件就绪，进程收到SIGIO信号，然后处理IO事件。
- 内核在第一个阶段是异步，在第二个阶段是同步；与非阻塞IO的区别在于它提供了消息通知机制，不需要用户进程不断的轮询检查，减少了系统API的调用次数，提高了效率。 

## 异步(asynchronous)
- Linux中，可以调用aio_read()函数告诉内核描述字缓冲区指针和缓冲区的大小、文件偏移及通知的方式，然后立即返回，当内核数据拷贝到缓冲区后，再通知应用程序。
```c
/* Asynchronous IO control block */
struct aiocb{
    int aio_fildes;  /* file desriptor */
    int aio_lio_opcode;  /* operation to be performed */
    int aio_reqrio;  /* request priority offset */
    volatile void *aio_buf;  /* location of buffer */
    size_t aio_nbytes;  /* length of transfer */
    struct sigevent aio_sigevent;  /* signal number and value */
} 
```

# 为什么TCP服务器的监听套接字要设置为非阻塞

我们一般使用IO复用来实现并发模型，如果我们默认监听套接字为阻塞模式，假设一种场景如下：
客户通过connect向TCP服务器发起三次握手

-     三次握手完成后，触发TCP服务器监听套接字的可读事件，IO复用返回（select、poll、epoll_wait）
-     客户通过RST报文取消连接
-     TCP服务器调用accept接受连接，此时发现内核已连接队列为空（因为唯一的连接已被客户端取消）
-     程序阻塞在accept调用，无法响应其它已连接套接字的事件

为了防止出现上面的场景，我们需要把监听套接字设置为非阻塞

# 硬链接和软链接的区别
- 硬链接指通过索引节点来进行连接 ，硬链接就是多个文件名指向同一个文件的索引节点。当删除原文件后，硬链接仍然可用
- 软链接也是符号链接，它更像一个快捷方式，它指向原文件，当删除原文件后，则软链接不可用。

# 多线程下的IO模型
- 线程角色固定
  - 线程不分角色
    - redis模式：单线程干3件事（夫妻店）
    - nginx模式：惊群模式（小饭馆）
  - 线程分角色
    - prooctor：主线程做12，工作线程做3
    - reactor：主线程做1，工作线程做23(Mina, Netty)
    - 主线程做1，工作线程做2，业务线程做3(Mina可配置)
    - 工作线程单独创造epollfd(memcached)
- 线程角色不固定
    - 领导者/追随者：Tengine(机场出租车)

# IO多路复用应用场景
## Ngnix支持IO多路复用模型
- select
- poll
- epoll: IO多路复用、高效并发模型，可在 Linux 2.6+ 及以上内核可以使用
- kqueue: IO多路复用、高效并发模型，可在 FreeBSD 4.1+, OpenBSD 2.9+, NetBSD 2.0, and Mac OS X 平台中使用
- /dev/poll: 高效并发模型，可在 Solaris 7 11/99+, HP/UX 11.22+ (eventport), IRIX 6.5.15+, and Tru64 UNIX 5.1A+ 平台使用
- eventport: 高效并发模型，可用于 Solaris 10 平台，PS：由于一些已知的问题，建议 使用/dev/poll替代。

## Redis支持IO多路复用模型
Redis 是跑在单线程中的，所有的操作都是按照顺序线性执行的，但是 由于读写操作等待用户输入或输出都是阻塞的，所以 I/O 操作在一般情况下往往不能直接返回，这会导致某一文件的 I/O 阻塞导致整个进程无法对其它客户提供服务，而 I/O 多路复用就是为了解决这个问题而出现

所谓 I/O 多路复用机制，就是说通过一种机制，可以监视多个描述符，一旦某个描述符就绪（一般是读就绪或写就绪），能够通知程序进行相应的读写操作。这种机制的使用需要 select 、 poll 、 epoll 来配合。多个连接共用一个阻塞对象，应用程序只需要在一个阻塞对象上等待，无需阻塞等待所有连接。当某条连接有新的数据可以处理时，操作系统通知应用程序，线程从阻塞状态返回，开始进行业务处理。 

Redis 服务采用 Reactor 的方式来实现文件事件处理器（每一个网络连接其实都对应一个文件描述符）。Redis基于Reactor模式开发了网络事件处理器，这个处理器被称为文件事件处理器。它的组成结构为4部分： 

- 多个套接字
- IO多路复用程序 
- 文件事件分派器 因为文件事件分派器队列的消费是单线程的，所以Redis才叫单线程模型
- 事件处理器。 

