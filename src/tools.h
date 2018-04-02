#ifndef SIMPLE_FTP_TOOLS_H
#define SIMPLE_FTP_TOOLS_H

#include <vector>
#include <string>

using namespace std;

string trim(string str); // 去掉头尾空白字符
vector<string> split(const string &str, const string &pattern); // 拆分字符串
vector<string> trim(vector<string> strs); // 将数组中的字符串都trim
vector<string> clean(vector<string> strs); // 去掉长度为0的字符串

string path_join(string a, string b); // 合并路径
string path_join(const string& base_path, const vector<string>& path); // 合并路径
string path_join(const string& base_path, const vector<string>& path, const string& step); // 合并路径
bool is_path_acceptable(const vector<string>& path); // 检查路径是否访问越界

struct Entry
{
    string name;
    size_t filesize;
    bool is_file;
    bool is_dir;
};
vector<Entry> listdir(const string& base_path, const vector<string>& path);

int mkdir(const string& final_path);
size_t filesize(const string& final_path);

bool is_dir(const string& final_path);
bool is_file(const string& final_path);
bool is_exists(const string& final_path);

#endif // SIMPLE_FTP_TOOLS_H