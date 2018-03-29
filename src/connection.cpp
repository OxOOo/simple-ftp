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
    hooks["cat"] = bind(&Connection::cat, this, std::placeholders::_1);
    hooks["recv"] = bind(&Connection::recv, this, std::placeholders::_1);
    hooks["up"] = bind(&Connection::up, this, std::placeholders::_1);
}

Connection::~Connection()
{
    delete[] buf;
}

void Connection::Init(int connfd, const string& base_path, bool cli_enable) // 初始化
{
    this->connfd = connfd;
    this->base_path = base_path;
    this->cli_enable = cli_enable;
    is_end = 0;
    pos = 0;
    path.clear();

    fd_to_send = NULL;
    close_after_sent = false;
    fd_to_write = NULL;
    
    CLI();
}

void Connection::Disconnect()
{
    if (fd_to_send != NULL) fclose(fd_to_send);
    fd_to_send = NULL;
    if (fd_to_write != NULL) fclose(fd_to_write);
    fd_to_write = NULL;
}

void Connection::Trigger(bool case_by_read)
{
    if (case_by_read)
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

            while(fd_to_write == NULL) // 当有文件要写入的时候不解析命令行
            {
                int newline = -1;
                for(newline = 0; newline < pos && buf[newline] != '\n'; newline ++);
                if (newline >= pos) break;
                Handle(buf, buf + newline);
                if (!MoreDataToWrite()) CLI();
                memcpy(buf, buf + newline + 1, pos - newline - 1);
                pos -= newline + 1;
            }
        }
        if (fd_to_write != NULL)
        {
            fwrite(buf, sizeof(char), pos, fd_to_write);
            pos = 0;
        }
    } else {
        if (fd_to_send == NULL)
        {
            LOG_FATAL << "nothing to write";
            is_end = 1;
            Writeln("error : nothing to write");
            return;
        }
        char* buffer = new char[BUFFSIZE];
        size_t n;
        if ((n = fread(buffer, sizeof(char), BUFFSIZE, fd_to_send)) <= 0)
        {
            fclose(fd_to_send);
            fd_to_send = NULL;
            CLI();
            if (close_after_sent) is_end = 1;
        } else {
            ::Write(connfd, buffer, n);
        }
        delete[] buffer;
    }
}

void Connection::CLI()
{
    if (!cli_enable) return;
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

bool Connection::ReadyToRead()
{
    return fd_to_send == NULL;
}

bool Connection::MoreDataToWrite()
{
    return fd_to_send != NULL;
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
            Writeln("[F] " + x.name + " " + to_string(x.filesize));
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
        if (!is_exists(path_join(base_path, path, args[1])))
        {
            ::mkdir(path_join(base_path, path, args[1]));
        }
    }
}

void Connection::cat(const vector<string>& args)
{
    if (args.size() != 2) {
        Writeln("error : cat takes exactly 1 argument");
    } else {
        string final_path = path_join(base_path, path, args[1]); // FIXME path check
        fd_to_send = fopen(final_path.c_str(), "rb");
        if (fd_to_send == NULL) Writeln("error : can not open file '" + args[1] + "'");
        close_after_sent = false;
    }
}

void Connection::recv(const vector<string>& args)
{
    if (args.size() != 2) {
        Writeln("error : recv takes exactly 1 argument");
    } else {
        string final_path = path_join(base_path, path, args[1]); // FIXME path check
        fd_to_send = fopen(final_path.c_str(), "rb");
        if (fd_to_send == NULL)
        {
            Writeln("error : can not open file '" + args[1] + "'");
            is_end = 1;
        }
        close_after_sent = true;
    }
}

void Connection::up(const vector<string>& args)
{
    if (args.size() != 2) {
        Writeln("error : up takes exactly 1 argument");
    } else {
        string final_path = path_join(base_path, path, args[1]); // FIXME path check
        fd_to_write = fopen(final_path.c_str(), "wb");
        if (fd_to_write == NULL)
        {
            Writeln("error : can not open file '" + args[1] + "'");
            is_end = 1;
        } else {
            Writeln("ready to recv file");
        }
    }
}
