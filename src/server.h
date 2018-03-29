#ifndef SIMPLE_FTP_SERVER_H
#define SIMPLE_FTP_SERVER_H

#include "unp.h"

class Server
{
public:
    Server();
    ~Server();

    void Loop();

private:
    socklen_t addrlen;
    int listenfd0, listenfd1;

    void Fork();
    void Progress();
};

#endif // SIMPLE_FTP_SERVER_H