# 本地套接字的作用：本地的进程间通信
- 有关系的进程间通信
- 没有关的进程件通信
本地套接字的实现流程与网络套接字的类似，一般使用tcp的流程

# 本地套接字的流程
## server
- 创建
    int lfd = socket(AF_UNIX/AF_LOCAL, SOCK_STREAM, 0);
- 监听的套接字绑定本地的套接字文件 -> server端
    struct sockadrr_un addr;
    // 绑定成功之后，指定的sun_path中的套接字文件会自动生成
    bind(lfd, addr, len);
- 监听
    listen(lfd, 100);
- 等待并接受连接请求
    struct sockaddr_in cliaddr;
    int cfd = accept(lfd, &cliaddr, len);
- 通信
    接收数据 read/recv
    发送数据 write/send
- 关闭连接
    close();

## client
- 创建
    int fd = socket(AF_UNIX/AF_LOCAL, SOCK_STREAM, 0);
- 监听的套接字绑定本地的IP端口
    struct sockaddr_un addr;
    // 绑定成功之后，指定的sun_path中的套接字文件会自动生成
    bind(fd, addr, len);
- 连接服务器
    struct sockaddr_un saddr;
    connect(fd, &saddr, len);
- 通信
    接收数据 read/recv
    发送数据 write/send
- 关闭连接
    close();