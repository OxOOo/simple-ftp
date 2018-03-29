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

    void Init(int connfd, const string& base_path, bool cli_enable); // 初始化

    void Trigger(bool case_by_read); // 触发响应

    void CLI(); // 输出命令行信息

    bool IsEnd(); // 是否链接断开

    bool ReadyToRead(); // 是否准备好读数据了

    bool MoreDataToWrite(); // 是否还有更多数据要写

private:
    int connfd, is_end;
    string base_path;
    bool cli_enable;
    char *buf;
    int pos;
    vector<string> path;

    FILE* fd_to_send; // 要发送的文件描述符
    bool close_after_sent;

    map<string, function<void(const vector<string>&)> > hooks;

    void Handle(const char* s, const char* t);
    void Write(const string& str);
    void Writeln(const string& str);

    void ls(const vector<string>& args);
    void cd(const vector<string>& args);
    void mkdir(const vector<string>& args);
    void cat(const vector<string>& args);
    void recv(const vector<string>& args);
};

#endif // SIMPLE_FTP_CONNECTION_H