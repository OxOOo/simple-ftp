#include "tools.h"
#include <cstring>

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
