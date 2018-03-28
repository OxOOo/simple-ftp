#include <plog/Log.h>
#include <plog/Appenders/ColorConsoleAppender.h>
#include "server.h"

using namespace std;

int main()
{
    static plog::ColorConsoleAppender<plog::TxtFormatter> consoleAppender;
    plog::init(plog::debug, &consoleAppender);

    LOG_INFO << "Hello World";

    Server s;
    s.Loop();

    return 0;
}
