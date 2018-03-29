#include "connection.h"
#include "tools.h"
#include <plog/Log.h>
#include <cassert>

using namespace std;

Connection::Connection()
{
    buf = new char[BUFFSIZE];
    hooks["ls"] = bind(&Connection::ls, this, std::placeholders::_1);
    hooks["cd"] = bind(&Connection::cd, this, std::placeholders::_1);
    hooks["mkdir"] = bind(&Connection::mkdir, this, std::placeholders::_1);
}

Connection::~Connection()
{
    delete[] buf;
}

void Connection::Init(int connfd, const string& base_path) // 初始化
{
    this->connfd = connfd;
    this->base_path = base_path;
    is_end = 0;
    pos = 0;
    path.clear();
    
    CLI();
}

void Connection::Trigger()
{
    if(pos == BUFFSIZE) // 缓存空间检查
    {
        LOG_FATAL << "full buffer";
        is_end = 1;
        Writeln("error : full buffer");
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
            CLI();
            memcpy(buf, buf + newline + 1, pos - newline - 1);
            pos -= newline + 1;
        }
    }
}

void Connection::CLI()
{
    for(const auto& x : path)
    {
        Write("/");
        Write(x);
    }
    if (path.size() == 0) Write("/");

    Write("$ ");
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
    args = trim(args);
    args = clean(args);

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
    if (args.size() != 1)
    {
        Writeln("error : ls takes exactly 1 argument");
        return;
    }

    vector<Entry> entries = listdir(base_path, path);
    for(const auto& x : entries)
    {
        if (x.is_file)
        {
            Writeln("[F] " + x.name);
        } else {
            if (path.size() > 0 || x.name != "..") Writeln("[D] " + x.name);
        }
    }
}

void Connection::cd(const vector<string>& args)
{
    if (args.size() == 1)
    {
        path.clear();
    } else if (args.size() == 2) {
        if (args[1] == "..")
        {
            if(path.size() > 0) path.pop_back();
        } else if (args[1] == ".") {
            // nothing
        } else {
            vector<string> old_path = path;
            string name = args[1];
            bool is_root = name[0] == '/';
            if (is_root) name = name.substr(1);

            vector<string> names = split(name, "/");
            names = trim(names);
            names = clean(names);
            if (is_root) path = names;
            else for(const auto&x : names) path.push_back(x);

            if (!is_dir(path_join(base_path, path))) path = old_path;
        }

    } else {
        Writeln("error : cd takes 0 or 1 argument(s)");
    }
}

void Connection::mkdir(const vector<string>& args)
{
    if (args.size() != 2) {
        Writeln("error : mkdir takes exactly 1 argument");
    } else {
        ::mkdir(path_join(base_path, path, args[1]));
    }
}
