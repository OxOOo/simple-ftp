#include "tools.h"
#include <plog/Log.h>
#include <cstring>
#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <algorithm>
#include <stack>

string trim(string str)
{
    if (str.empty())
    {
        return str;
    }

    str.erase(0, str.find_first_not_of(" "));
    str.erase(str.find_last_not_of(" ") + 1);
    return str;
}

vector<string> split(const string &str, const string &pattern)
{
    //const char* convert to char*
    char *strc = new char[str.length() + 1];
    strcpy(strc, str.c_str());
    vector<string> resultVec;
    char *tmpStr = strtok(strc, pattern.c_str());
    while (tmpStr != NULL)
    {
        resultVec.push_back(string(tmpStr));
        tmpStr = strtok(NULL, pattern.c_str());
    }

    delete[] strc;

    return resultVec;
}

vector<string> trim(vector<string> strs)
{
    for(auto&x : strs) x = trim(x);
    return strs;
}

vector<string> clean(vector<string> strs)
{
    int size = 0;
    for(int i = 0; i < (int)strs.size(); i ++)
        if (strs[i].length() != 0)
            strs[size++] = strs[i];
    return strs;
}


string path_join(string a, string b)
{
    if (a[a.length()-1] == '/') a = a.substr(0, a.length()-1);
    if (b[0] == '/') b = b.substr(1);
    if (b[b.length()-1] == '/') b = b.substr(0, b.length()-1);
    return a + "/" + b;
}

string path_join(const string& base_path, const vector<string>& path)
{
    string final_path = base_path;
    for(const auto& x : path)
    {
        final_path = path_join(final_path, x);
    }

    return final_path;
}

string path_join(const string& base_path, const vector<string>& path, const string& step)
{
    return path_join(path_join(base_path, path), step);
}

bool is_path_acceptable(const vector<string>& path)
{
    stack<string> s;
    for(const auto& x : path)
    {
        for(int i = 0; i < x.length(); i ++)
            if (x[i] == '/' || x[i] == '\\') // 每个路径节点不允许[/\]
                return false;
        if (x == ".") continue;
        if (x == ".." && s.empty()) return false;
        if (x == "..") s.pop(); else s.push(x);
    }
    return true;
}

vector<Entry> listdir(const string& base_path, const vector<string>& path)
{
    string final_path = path_join(base_path, path);
    
    vector<Entry> rst;
    DIR* dir = opendir(final_path.c_str());
    if (dir == NULL) return rst;

    struct dirent* ptr;
    while((ptr = readdir(dir)) != NULL)
    {
        if (ptr->d_type & (DT_REG | DT_DIR))
        {
            Entry e;
            e.name = ptr->d_name;
            e.is_file = ptr->d_type & DT_REG;
            e.is_dir = ptr->d_type & DT_DIR;
            e.filesize = e.is_file ? filesize(path_join(final_path, e.name)) : 0;
            rst.push_back(e);
        }
    }

    closedir(dir);

    sort(rst.begin(), rst.end(), [](const Entry& a, const Entry& b) {
        return a.name < b.name;
    });

    return rst;
}

int mkdir(const string& final_path)
{
    return mkdir(final_path.c_str(), 0775);
}

size_t filesize(const string& final_path)
{
    struct stat buf;
    if (stat(final_path.c_str(), &buf) < 0) return 0;
    return buf.st_size;
}

bool is_dir(const string& final_path)
{
    struct stat buf;
    if (stat(final_path.c_str(), &buf) < 0) return false;
    return buf.st_mode & S_IFDIR;
}

bool is_file(const string& final_path)
{
    struct stat buf;
    if (stat(final_path.c_str(), &buf) < 0) return false;
    return buf.st_mode & S_IFREG;
}

bool is_exists(const string& final_path)
{
    struct stat buf;
    if (stat(final_path.c_str(), &buf) < 0) return false;
    return true;
}