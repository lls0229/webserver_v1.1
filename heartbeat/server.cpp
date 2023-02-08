/**
 * File Name: server.cpp
 * Author: lls
 * E-mail: lls840308420@163.com
 * Created: 
*/

#include<iostream>
#include<netinet/in.h>
#include<sys/socket.h>
#include<sys/types.h>
#include<arpa/inet.h>
#include<unistd.h>
#include<sys/select.h>
#include<sys/ioctl.h>
#include<sys/time.h>
#include<map>
#include<vector>
#include<string>
#include<cstdlib>
#include<cstring>
#include<cstdio>
#define BUFFER_SIZE_ 1024
enum Type{ HEART, OTHER };

struct PACKET_HEAD{

    Type type;
    int length;
};

void *heart_handler(void *arg);

class Server{

private:
    struct sockaddr_in s_addr;
    socklen_t s_addr_len;
    int listen_fd;  // fd being listened
    int max_fd;  // max fd
    fd_set master_set;  // the set of all fds, including lfd and cfd
    fd_set working_set;  // the set of workings
    struct timeval timeout;
    std::map<int, std::pair<std::string, int>> mmap;  // regard the connected client's ip and count

public:
    Server(int port);
    ~Server();
    void Bind();
    void Listen(int queue_len = 20);
    void Accept();
    void Run();
    void Recv(int nums);
    friend void *heart_handler(void *arg);
};

Server::Server(int port){

    bzero(&s_addr, sizeof(s_addr));
    s_addr.sin_family = AF_INET;
    s_addr.sin_port = htons(port);
    s_addr.sin_addr.s_addr = htons(INADDR_ANY);
    
    /* create socket to listen */
    listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if(listen_fd < 0){
        perror("socket");
        exit(-1);
    }

    int opt = 1;
    setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
}

Server::~Server(){

    for(int fd = 0; fd <= max_fd; ++fd){
        if(FD_ISSET(fd, &master_set)){
            close(fd);
        }
    }
}

void Server::Bind(){

    if(bind(listen_fd, (struct sockaddr *)&s_addr, sizeof(s_addr)) == -1){
        perror("bind");
        exit(-2);
    }
    std::cout << "bind successfully"  << std::endl;
}

void Server::Listen(int queue_len){

    if(listen(listen_fd, queue_len) == -1){
        perror("listen");
        exit(-3);
    }
    std::cout << "listen successfully" << std::endl;
}

void Server::Accept(){

    struct sockaddr_in c_addr;
    socklen_t c_addr_len = sizeof(c_addr);

    int client_fd = accept(listen_fd, (struct sockaddr *)&c_addr, &c_addr_len);
    if(client_fd < 0){
        perror("accept");
        exit(-4);
    }
    
    std::string ip(inet_ntoa(c_addr.sin_addr));  // get client's ip
    std::cout << ip << "new connection was accepted." << std::endl;

    mmap.insert(std::make_pair(client_fd, std::make_pair(ip, 0)));
    
    /* add freshly connected fd to master_set */
    FD_SET(client_fd, &master_set);
    if(client_fd > max_fd){
        max_fd = client_fd;
    }
}

void Server::Recv(int nums){

    for(int fd = 0; fd <= max_fd; ++fd){
        if(FD_ISSET(fd, &working_set)){
            bool close_conn = false;  /* flag current connection disconnect or not */

            PACKET_HEAD head;
            recv(fd, &head, sizeof(head), 0);  /* firstly accept the header */

            if(head.type == HEART){
                mmap[fd].second = 0;  /* once accept hb, set count as 0 */
                std::cout << "Received heart-beat from client." << std::endl;        
            }
            else {
                /* via head.length conferen the length */
            }

            if(close_conn){
                /* the current connection is wrong, close it */
                close(fd);
                FD_CLR(fd, &master_set);
                if(fd == max_fd){
                    while(FD_ISSET(max_fd, &master_set) == false){
                        --max_fd;
                    }
                }
            }
        
        
        }
    }
}

void Server::Run(){

    pthread_t id;  /* create thread to check heart-beat */
    int ret = pthread_create(&id, NULL, heart_handler, (void *)this);
    if(ret != 0){
        std::cout << "Can not create heart-beat checking thread." << std::endl;
    }

    max_fd = listen_fd;  /* init max_fd */
    FD_ZERO(&master_set);
    FD_SET(listen_fd, &master_set);  /* add listen-fd */

    while(1){
        FD_ZERO(&working_set);
        memcpy(&working_set, &master_set, sizeof(master_set));
 
        timeout.tv_sec = 30;
        timeout.tv_usec = 0;
 
        int nums = select(max_fd+1, &working_set, NULL, NULL, &timeout);
        if(nums < 0)
        {
            std::cout << "select() error!";
            exit(1);
        }
 
        if(nums == 0)
        {
            //cout << "select() is timeout!";
            continue;
        }
 
        if(FD_ISSET(listen_fd, &working_set))
            Accept();   /* new client request */
        else
            Recv(nums); /* recvice client's msg */
    }
}

/* thread function */
void *heart_handler(void *arg){
    std::cout << "The heartbeat checking thread started."  <<std::endl;
    Server *s = (Server *)arg;
    while(1){
        std::map<int, std::pair<std::string, int>>::iterator it = s->mmap.begin();
        for(; it != s->mmap.end(); ){
            if(it->second.second == 5){
                /* if 3s*5 do not accept hb, the client is outline */
                std::cout << "The client " << it->second.first << " has be offline." << std::endl;

                int fd = it->first;
                close(fd);  /* close the connection */
                FD_CLR(fd, &s->master_set);
                if(fd == s->max_fd){
                    /* need update max_fd */
                    while(FD_ISSET(s->max_fd, &s->master_set) == false)
                        s->max_fd --;
                }

                s->mmap.erase(it++);  /* delete the regard from map */
            }
            else if(it->second.second < 5 && it->second.second >= 0){
                it->second.second += 1;
                ++ it;
            }
            else {
                ++ it;
            }
        }
        sleep(3);  /* alock 3 secs */
    }

}

int main(){

    Server server(9999);
    server.Bind();
    server.Listen();
    server.Run();
    return 0;
}
