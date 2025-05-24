#pragma once

#include <string>
#include <stdint.h>
#include <memory>

namespace sylar
{

    class LogEvent
    {
    public:
        typedef std::shared_ptr<LogEvent> ptr;
        LogEvent();

    private:
        const char *m_file = nullptr;
        int32_t m_line = 0;
        uint32_t m_elapse = 0;
        int32_t m_threadId = 0;
        uint32_t m_fiberId = 0;
        uint64_t m_time = 0;
        std::string m_content;
    };

    enum class LogLevel
    {
        DEBUG = 1,
        INFO = 2,
        WARN = 3,
        ERROR = 4,
        FATAL = 5
    };

    class LogFormatter
    {
    public:
        typedef std::shared_ptr<LogFormatter> ptr;

        std::string format(LogEvent::ptr event);
    };

    class LogAppender
    {
    public:
        typedef std::shared_ptr<LogAppender> ptr;
        virtual ~LogAppender() = default;

        void log(LogLevel level, LogEvent::ptr event);

    private:
        LogLevel m_level;
    };

    class Logger
    {
    public:
        typedef std::shared_ptr<Logger> ptr;

        Logger(const std::string &name = "root");

        void log(LogLevel level, LogEvent::ptr event);

    private:
        std::string m_name;
        LogLevel m_level;
        LogAppender::ptr m_appender;
    };

    // log appender for terminal
    class StdoutLogAppender : public LogAppender
    {
    };

    // log appender for file
    class FileLogAppender : public LogAppender
    {
    };

}