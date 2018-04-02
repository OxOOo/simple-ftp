#include "server.h"
#include "connection.h"
#include "unp.h"
#include <plog/Log.h>
#include <cstdlib>

#define PROCESSES 8
#define MAX_EVENTS 1024 // 每个进程最多同时处理的事件数量,基于poll
#define BASE_PATH "../repo"

Server::Server()
{
}

Server::~Server()
{
}

void Server::Loop()
{
    listenfd0 = Tcp_listen("0.0.0.0", "4000", &addrlen);
    listenfd1 = Tcp_listen("0.0.0.0", "4001", &addrlen);

    // 不处理SIGPIPE信号
    Signal(SIGPIPE, SIG_IGN);

    for (int i = 0; i < PROCESSES; i++)
    {
        Fork();
    }

    while(1)
    {
        int status;
        pid_t pid = Waitpid(0, &status, 0);
        LOG_ERROR << "pid=" << pid << " exit with status = " << status;
        Fork();
    }
}

void Server::Fork()
{
    pid_t pid;
    if ((pid = fork()) < 0)
    {
        LOG_FATAL << "error on fork";
        exit(1);
    }
    if (pid == 0)
    { // child
        LOG_INFO << "forked child pid=" << getpid();

        Progress();
        
        exit(0);
    }
}

void Server::Progress()
{
    struct pollfd* fds = (struct pollfd *)Malloc(sizeof(struct pollfd)*MAX_EVENTS);
    Connection *socks = new Connection[MAX_EVENTS];
    for(int i = 0; i < MAX_EVENTS; i ++)
    {
        fds[i].fd = -1;
        fds[i].events = fds[i].revents = 0;
    }
    struct sockaddr* cliaddr = (struct sockaddr*)Malloc(addrlen);

    fds[0].fd = listenfd0;
    fds[1].fd = listenfd1;

    int maxi = 1; // 最大位置
    int emptys = MAX_EVENTS - 2; // 空闲的数量
    while(1)
    {
        while(maxi && fds[maxi].fd < 0) maxi --;
        if (emptys == 0) fds[0].events = fds[1].events = 0; // 如果该进程能够处理的数量太多,则不Accept新链接
        else fds[0].events = fds[1].events = POLLRDNORM;

        int nready = Poll(fds, maxi + 1, INFTIM);

        for(int i = 0; i < 2; i ++)
        {
            if (fds[i].revents & POLLRDNORM)
            {
                socklen_t clilen = addrlen;
                bzero(cliaddr, addrlen);
                int connfd = Accept(fds[i].fd, (SA*)cliaddr, &clilen);

                int pos = -1;
                for(pos = 2; fds[pos].fd >= 0; pos ++);
                fds[pos].fd = connfd;
                socks[pos].Init(connfd, BASE_PATH, i == 0);
                fds[pos].events = 0;
                if (socks[pos].ReadyToRead()) fds[pos].events |= POLLRDNORM;
                if (socks[pos].MoreDataToWrite()) fds[pos].events |= POLLWRNORM;
                if (maxi < pos) maxi = pos;
                emptys --;
                LOG_DEBUG << "new connection to listenfd" << i << " fd = " << connfd << " pos = " << pos;
                --nready;
            }
        }

        if (nready == 0) continue;

        for(int i = 2; i <= maxi; i ++)
        {
            int connfd = fds[i].fd;
            if (connfd < 0) continue;
            if (fds[i].revents & (POLLRDNORM | POLLERR))
            {
                socks[i].Trigger(true); // 可读数据
            }
            if (fds[i].revents & (POLLWRNORM | POLLERR))
            {
                socks[i].Trigger(false); // 可写数据
            }
            if (fds[i].revents != 0)
            {
                fds[i].events = 0;
                if (socks[i].IsEnd())
                {
                    LOG_DEBUG << "disconnect fd = " << fds[i].fd << " pos = " << i;
                    Close(fds[i].fd);
                    socks[i].Disconnect();
                    fds[i].fd = -1; // 删除链接
                    emptys ++;
                } else {
                    if (socks[i].ReadyToRead()) fds[i].events |= POLLRDNORM;
                    if (socks[i].MoreDataToWrite()) fds[i].events |= POLLWRNORM;
                }
            }
        }
    }

    delete[] socks;
    free(fds);
    free(cliaddr);
}