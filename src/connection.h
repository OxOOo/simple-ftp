#ifndef SIMPLE_FTP_CONNECTION_H
#define SIMPLE_FTP_CONNECTION_H

#include "unp.h"
#include <vector>
#include <string>
#include <map>
#include <functional>

using namespace std;

class Connection
{
public:
    Connection();
    ~Connection();

    void Init(int connfd); // 初始化

    void Trigger(); // 触发响应

    bool IsEnd(); // 是否链接断开

private:
    int connfd, is_end;
    char *buf;
    int pos;

    map<string, function<void(const vector<string>&)> > hooks;

    void Handle(const char* s, const char* t);
    void Write(const string& str);
    void Writeln(const string& str);

    void ls(const vector<string>& args);
};

#endif // SIMPLE_FTP_CONNECTION_H