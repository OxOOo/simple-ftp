#include "server.h"
#include "connection.h"
#include "unp.h"
#include <plog/Log.h>
#include <cstdlib>

#define PROCESSES 1
#define MAX_EVENTS 1024 // 每个进程最多同时处理的事件数量,基于poll

Server::Server()
{
}

Server::~Server()
{
}

void Server::Loop()
{
    listenfd = Tcp_listen("0.0.0.0", "4000", &addrlen);

    for (int i = 0; i < PROCESSES; i++)
    {
        Fork();
    }

    while(1)
    {
        pid_t pid = Wait(NULL);
        LOG_ERROR << "pid=" << pid << " exit";
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
    struct pollfd *fds = (struct pollfd *)Malloc(sizeof(struct pollfd)*MAX_EVENTS);
    Connection *socks = new Connection[MAX_EVENTS];
    for(int i = 0; i < MAX_EVENTS; i ++)
    {
        fds[i].fd = -1;
        fds[i].events = fds[i].revents = 0;
    }
    struct sockaddr *cliaddr = (struct sockaddr *)Malloc(addrlen);
    socklen_t clilen;

    fds[0].fd = listenfd;

    int maxi = 0; // 最大位置
    int emptys = MAX_EVENTS - 1; // 空闲的数量
    while(1)
    {
        while(maxi && fds[maxi].fd < 0) maxi --;
        if (emptys == 0) fds[0].events = 0; // 如果该进程能够处理的数量太多,则不Accept新链接
        else fds[0].events = POLLRDNORM;

        int nready = Poll(fds, maxi + 1, INFTIM);

        if (fds[0].revents & POLLRDNORM)
        {
            int connfd = Accept(listenfd, cliaddr, &clilen);
            int pos = -1;
            for(pos = 1; fds[pos].fd >= 0; pos ++);
            fds[pos].fd = connfd;
            fds[pos].events = POLLRDNORM;
            socks[pos].Init(connfd);
            if (maxi < pos) maxi = pos;
            emptys --;
            LOG_DEBUG << "new connection fd = " << connfd << " pos = " << pos;
            if (--nready <= 0) continue;
        }

        for(int i = 1; i <= maxi; i ++)
        {
            int connfd = fds[i].fd;
            if (connfd < 0) continue;
            if (fds[i].revents & (POLLRDNORM | POLLERR))
            {
                socks[i].Trigger(); // 触发
                if (socks[i].IsEnd())
                {
                    LOG_DEBUG << "disconnect fd = " << fds[i].fd << " pos = " << i;
                    Close(fds[i].fd);
                    fds[i].fd = -1; // 删除链接
                }
            }
        }
    }

    delete[] socks;
    free(fds);
    free(cliaddr);
}