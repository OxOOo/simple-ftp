#ifndef SIMPLE_FTP_TOOLS_H
#define SIMPLE_FTP_TOOLS_H

#include <vector>
#include <string>

using namespace std;

string trim(string str);
vector<string> split(const string &str, const string &pattern);
vector<string> trim(vector<string> strs);
vector<string> clean(vector<string> strs);

string path_join(string a, string b);
string path_join(const string& base_path, const vector<string>& path);
string path_join(const string& base_path, const vector<string>& path, const string& step);

struct Entry
{
    string name;
    bool is_file;
    bool is_dir;  
};
vector<Entry> listdir(const string& base_path, const vector<string>& path);

int mkdir(const string& final_path);

bool is_dir(const string& final_path);
bool is_file(const string& final_path);
bool is_exists(const string& final_path);

#endif // SIMPLE_FTP_TOOLS_H