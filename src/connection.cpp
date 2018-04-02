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
    hooks["rm"] = bind(&Connection::rm, this, std::placeholders::_1);
    hooks["rmdir"] = bind(&Connection::rmdir, this, std::placeholders::_1);
    hooks["cat"] = bind(&Connection::cat, this, std::placeholders::_1);
    hooks["recv"] = bind(&Connection::recv, this, std::placeholders::_1);
    hooks["up"] = bind(&Connection::up, this, std::placeholders::_1);
    hooks["help"] = bind(&Connection::help, this, std::placeholders::_1);
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
    
    Banner();
    CLI();
}

void Connection::Disconnect()
{
    if (fd_to_send != NULL) fclose(fd_to_send);
    fd_to_send = NULL;
    if (fd_to_write != NULL) fclose(fd_to_write);
    fd_to_write = NULL;
}

void Connection::Banner()
{
    Writeln("欢迎来到 simple-ftp");
    Writeln("输入help获取帮助信息");
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

            while(fd_to_write == NULL) // 当有文件要写入的时候不解析命令行,否则的话查看当前缓冲区中是否存在\n
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
        Writeln("error : ls takes exactly 0 argument");
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
        if (args[1] == "..") // 移动到上层目录
        {
            if(path.size() > 0) path.pop_back();
        } else if (args[1] == ".") { // 不移动
            // nothing
        } else { // 目录切换
            vector<string> old_path = path; // 备份原目录
            string name = args[1];
            bool is_root = name[0] == '/';
            if (is_root) name = name.substr(1);

            vector<string> names = split(name, "/");
            names = trim(names);
            names = clean(names);
            if (is_root) path = names;
            else for(const auto&x : names) path.push_back(x);

            if (!is_path_acceptable(path)) { // 如果路径不合法则不切换
                path = old_path;
                Writeln("error : path is not acceptable");
            } else if (!is_dir(path_join(base_path, path))) { // 如果路径不存在则不切换
                path = old_path;
                Writeln("error : path is not exists");
            }
        }

    } else {
        Writeln("error : cd takes 0 or 1 argument");
    }
}

void Connection::mkdir(const vector<string>& args)
{
    if (args.size() != 2) {
        Writeln("error : mkdir takes exactly 1 argument");
    } else {
        vector<string> new_path;
        new_path = path;
        new_path.push_back(args[1]);

        if (!is_path_acceptable(new_path)) // 如果路径不合法则不创建
        {
            Writeln("error : path is not acceptable");
            return;
        }

        if (!is_exists(path_join(base_path, new_path))) // 目录不存在则创建
        {
            ::mkdir(path_join(base_path, new_path));
        } else {
            Writeln("error : path already exists");
            return;
        }
    }
}

void Connection::rm(const vector<string>& args)
{
    if (args.size() != 2) {
        Writeln("error : rm takes exactly 1 argument");
    } else {
        vector<string> new_path;
        new_path = path;
        new_path.push_back(args[1]);

        if (!is_path_acceptable(new_path))
        {
            Writeln("error : path is not acceptable");
            return;
        }

        if (is_file(path_join(base_path, new_path))) // 文件存在
        {
            ::rm(path_join(base_path, new_path));
        } else {
            Writeln("error : path does not exists or path is not a file");
            return;
        }
    }
}

void Connection::rmdir(const vector<string>& args)
{
    if (args.size() != 2) {
        Writeln("error : rmdir takes exactly 1 argument");
    } else {
        vector<string> new_path;
        new_path = path;
        new_path.push_back(args[1]);

        if (!is_path_acceptable(new_path))
        {
            Writeln("error : path is not acceptable");
            return;
        }

        if (is_dir(path_join(base_path, new_path))) // 目录存在
        {
            ::rmdir(path_join(base_path, new_path));
        } else {
            Writeln("error : path does not exists or path is not a dir");
            return;
        }
    }
}

void Connection::cat(const vector<string>& args)
{
    if (args.size() != 2) {
        Writeln("error : cat takes exactly 1 argument");
    } else {
        vector<string> new_path = path;

        string name = args[1];
        bool is_root = name[0] == '/';
        if (is_root) name = name.substr(1);
        vector<string> names = split(name, "/");
        names = trim(names);
        names = clean(names);
        if (is_root) new_path = names;
        else for(const auto&x : names) new_path.push_back(x);

        if (!is_path_acceptable(new_path)) // 如果路径不合法
        {
            Writeln("error : path is not acceptable");
            return;
        }

        string final_path = path_join(base_path, new_path);
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
        vector<string> new_path = path;

        string name = args[1];
        bool is_root = name[0] == '/';
        if (is_root) name = name.substr(1);
        vector<string> names = split(name, "/");
        names = trim(names);
        names = clean(names);
        if (is_root) new_path = names;
        else for(const auto&x : names) new_path.push_back(x);

        if (!is_path_acceptable(new_path)) // 如果路径不合法
        {
            Writeln("error : path is not acceptable");
            return;
        }

        string final_path = path_join(base_path, new_path);
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
        vector<string> new_path = path;

        string name = args[1];
        bool is_root = name[0] == '/';
        if (is_root) name = name.substr(1);
        vector<string> names = split(name, "/");
        names = trim(names);
        names = clean(names);
        if (is_root) new_path = names;
        else for(const auto&x : names) new_path.push_back(x);

        if (!is_path_acceptable(new_path)) // 如果路径不合法
        {
            Writeln("error : path is not acceptable");
            return;
        }

        string final_path = path_join(base_path, new_path);
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

void Connection::help(const vector<string>& args)
{
    Writeln("ls : 展示当前目录下的文件");
    Writeln("cd path : 切换目录");
    Writeln("mkdir path : 创建目录");
    Writeln("rm path : 删除文件");
    Writeln("rmdir path : 递归删除目录");
    Writeln("cat path : 输出文件内容");
    Writeln("recv path : 输出文件内容,并且输出完毕之后关闭链接");
    Writeln("up path : 上传文件到path中");
    Writeln("help : 显示帮助文档");
}
