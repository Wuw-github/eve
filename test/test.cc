#include "log.h"
#include "util.h"
int main(int argc, char **argv)
{
    using namespace sylar;

    Logger::ptr logger(new Logger);

    logger->addAppender(LogAppender::ptr(new StdoutLogAppender));

    FileLogAppender::ptr fileAppender(new FileLogAppender("./log.txt"));
    LogFormatter::ptr fmt(new LogFormatter("%d%T%p%T%m%n"));
    fileAppender->setFormatter(fmt);
    fileAppender->setLevel(LogLevel::ERROR);

    logger->addAppender(fileAppender);

    LOG_INFO(logger) << "Hello World";
    LOG_FMT_ERROR(logger, "Hello %s Error", "World");
    return 0;
}