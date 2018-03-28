#include "connection.h"
#include "tools.h"
#include <plog/Log.h>
#include <cassert>

using namespace std;

Connection::Connection()
{
    buf = new char[BUFFSIZE];
    hooks["ls"] = bind(&Connection::ls, this, std::placeholders::_1);
}

Connection::~Connection()
{
    delete[] buf;
}

void Connection::Init(int connfd) // 初始化
{
    this->connfd = connfd;
    is_end = 0;
    pos = 0;
}

void Connection::Trigger()
{
    if(pos == BUFFSIZE) // 缓存空间检查
    {
        LOG_FATAL << "full buffer";
        is_end = 1; // FIXME need error message
        return;
    }

    ssize_t n = read(connfd, buf+pos, BUFFSIZE-pos);
    if (n < 0)
    {
        if (errno == ECONNRESET)
        {
            is_end = 1;
        } else LOG_ERROR << "read error";
    } else if (n == 0)
    {
        is_end = 1;
    } else
    {
        pos += n;

        while(true)
        {
            int newline = -1;
            for(newline = 0; newline < pos && buf[newline] != '\n'; newline ++);
            if (newline >= pos) break;
            Handle(buf, buf + newline);
            memcpy(buf, buf + newline + 1, pos - newline - 1);
            pos -= newline + 1;
        }
    }
}

bool Connection::IsEnd()
{
    return is_end;
}

void Connection::Handle(const char* s, const char* t)
{
    while(s <= t && isspace(*t)) t --;
    if (s > t) return;

    vector<string> args = split(string(s, t-s+1), " ");
    for(auto& x : args)
    {
        x = trim(x);
    }
    int size = 0;
    for(int i = 0; i < (int)args.size(); i ++)
        if (args[i].length() != 0)
            args[size++] = args[i];
    args.resize(size);

    if (args.size() > 0)
    {
        if (hooks.find(args[0]) == hooks.end())
        {
            Writeln("unknow command [" + string(s, t-s+1) + "]");
        } else {
            hooks.at(args[0])(args);
        }
    }

    // FIXME something
}

void Connection::Write(const string& str)
{
    ::Write(connfd, str.c_str(), str.length());
}

void Connection::Writeln(const string& str)
{
    Write(str + "\n");
}

void Connection::ls(const vector<string>& args)
{
    Writeln("LS");
}
