#pragma once

#include <string>
#include <stdint.h>
#include <memory>
#include <list>
#include <sstream>
#include <fstream>
#include <iostream>
#include <vector>
#include <map>

#include "util.h"
#include "singleton.h"
#include "mutex.h"
#include "thread.h"

#define LOG_LEVEL(logger, level)     \
    if (logger->getLevel() <= level) \
    sylar::LogEventWrap(sylar::LogEvent::ptr(new sylar::LogEvent(logger, level, __FILE__, __LINE__, 0, GetThreadId(), Thread::GetName(), GetFiberId(), time(0), ""))).getSS()

#define LOG_DEBUG(logger) LOG_LEVEL(logger, LogLevel::DEBUG)
#define LOG_INFO(logger) LOG_LEVEL(logger, LogLevel::INFO)
#define LOG_WARN(logger) LOG_LEVEL(logger, LogLevel::WARN)
#define LOG_ERROR(logger) LOG_LEVEL(logger, LogLevel::ERROR)
#define LOG_FATAL(logger) LOG_LEVEL(logger, LogLevel::FATAL)

#define LOG_FMT_LEVEL(logger, level, fmt, ...) \
    if (logger->getLevel() <= level)           \
    sylar::LogEventWrap(LogEvent::ptr(new sylar::LogEvent(logger, level, __FILE__, __LINE__, 0, GetThreadId(), Thread::GetName(), GetFiberId(), time(0), fmt))).getEvent()->format(fmt, __VA_ARGS__)

#define LOG_FMT_DEBUG(logger, fmt, ...) LOG_FMT_LEVEL(logger, LogLevel::DEBUG, fmt, __VA_ARGS__)
#define LOG_FMT_INFO(logger, fmt, ...) LOG_FMT_LEVEL(logger, LogLevel::INFO, fmt, __VA_ARGS__)
#define LOG_FMT_WARN(logger, fmt, ...) LOG_FMT_LEVEL(logger, LogLevel::WARN, fmt, __VA_ARGS__)
#define LOG_FMT_ERROR(logger, fmt, ...) LOG_FMT_LEVEL(logger, LogLevel::ERROR, fmt, __VA_ARGS__)
#define LOG_FMT_FATAL(logger, fmt, ...) LOG_FMT_LEVEL(logger, LogLevel::FATAL, fmt, __VA_ARGS__)

#define LOG_ROOT() sylar::LoggerMgr::GetInstance().getRoot()
#define LOG_NAME(name) sylar::LoggerMgr::GetInstance().getLogger(name)

namespace sylar
{

    class Logger;
    class LoggerManager;

    class LogLevel
    {
    public:
        enum Level
        {
            UNKNOWN = 0,
            DEBUG = 1,
            INFO = 2,
            WARN = 3,
            ERROR = 4,
            FATAL = 5
        };

        static const char *ToString(LogLevel::Level level);
        static Level FromString(const std::string &str);
    };

    class LogEvent
    {
    public:
        typedef std::shared_ptr<LogEvent> ptr;
        LogEvent(std::shared_ptr<Logger> logger, LogLevel::Level level, const char *file, int32_t m_line, uint32_t elapse,
                 int32_t threadId, std::string threadName, uint32_t fiberId, uint64_t time, const std::string &content);
        ~LogEvent();
        const char *getFile() const { return m_file; }
        int32_t getLine() const { return m_line; }
        uint32_t getElapse() const { return m_elapse; }
        int32_t getThreadId() const { return m_threadId; }
        const std::string &getThreadName() const { return m_threadName; }
        uint32_t getFiberId() const { return m_fiberId; }
        uint64_t getTime() const { return m_time; }
        std::string getContent() const { return m_content.str(); }
        std::stringstream &getSS() { return m_content; }
        std::shared_ptr<Logger> getLogger() const { return m_logger; }
        LogLevel::Level getLevel() const { return m_level; }

        void format(const char *fmt, ...);
        void format(const char *fmt, va_list al);

    private:
        LogLevel::Level m_level;
        const char *m_file = nullptr;
        int32_t m_line = 0;

        uint32_t m_elapse = 0;
        int32_t m_threadId = 0;
        uint32_t m_fiberId = 0;
        uint64_t m_time = 0;
        std::stringstream m_content;
        std::string m_threadName;

        std::shared_ptr<Logger> m_logger;
    };

    class LogEventWrap
    {

    public:
        LogEventWrap(LogEvent::ptr e);
        ~LogEventWrap();
        std::stringstream &getSS() { return m_event->getSS(); }
        LogEvent::ptr getEvent() { return m_event; }

    private:
        LogEvent::ptr m_event;
    };

    class LogFormatter
    {
    public:
        typedef std::shared_ptr<LogFormatter> ptr;
        LogFormatter(const std::string pattern);

        // %t     %thread_id
        virtual std::string format(std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event);

        void init();

        bool isError() { return m_error; }

        std::string getPattern() { return m_pattern; }

    public:
        class FormatItem
        {
        public:
            typedef std::shared_ptr<FormatItem> ptr;
            FormatItem(const std::string &str = "") {}
            virtual ~FormatItem() {}
            virtual void format(std::ostream &os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) = 0;
        };

    private:
        std::string m_pattern;
        std::vector<FormatItem::ptr> m_items;
        bool m_error{false};
    };

    class LogAppender
    {
        friend class Logger;

    public:
        typedef std::shared_ptr<LogAppender> ptr;
        typedef Mutex MutexType;
        virtual ~LogAppender() = default;

        virtual void log(std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) = 0;
        void setFormatter(LogFormatter::ptr formatter);
        LogFormatter::ptr getFormatter();

        void setLevel(LogLevel::Level level) { m_level = level; }
        LogLevel::Level getLevel() const { return m_level; }

        virtual std::string toYamlString() = 0;

    protected:
        LogLevel::Level m_level;
        MutexType m_mutex;
        LogFormatter::ptr m_formatter;
    };

    class Logger : public std::enable_shared_from_this<Logger>
    {
    public:
        friend class LoggerManager;
        typedef std::shared_ptr<Logger> ptr;
        typedef Mutex MutexType;

        Logger(const std::string &name = "root");

        void log(LogLevel::Level level, LogEvent::ptr event);
        void debug(LogEvent::ptr event);
        void info(LogEvent::ptr event);
        void warn(LogEvent::ptr event);
        void error(LogEvent::ptr event);
        void fatal(LogEvent::ptr event);

        void addAppender(LogAppender::ptr appender);

        void delAppender(LogAppender::ptr appender);

        void clearAppenders();

        LogLevel::Level getLevel() const;

        void setLevel(LogLevel::Level level);

        const std::string &getName() const { return m_name; }

        void setFormatter(LogFormatter::ptr val);
        void setFormatter(const std::string &val);

        LogFormatter::ptr getFormatter() { return m_formatter; };

        std::string toYamlString();

    private:
        std::string m_name;
        LogLevel::Level m_level{LogLevel::UNKNOWN};
        MutexType m_mutex;
        std::list<LogAppender::ptr> m_appender;
        LogFormatter::ptr m_formatter;
        Logger::ptr m_root;
    };

    // log appender for terminal
    class StdoutLogAppender : public LogAppender
    {
    public:
        typedef std::shared_ptr<StdoutLogAppender> ptr;
        void log(Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override;

        std::string toYamlString() override;

    private:
    };

    // log appender for file
    class FileLogAppender : public LogAppender
    {
    public:
        typedef std::shared_ptr<FileLogAppender> ptr;
        FileLogAppender(const std::string &filename);

        void log(Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override;

        bool reopen();

        std::string toYamlString() override;

    private:
        std::string m_filename;
        std::ofstream m_filestream;
        uint64_t m_lastOpenTime{0};
    };

    class LoggerManager
    {
    public:
        typedef Mutex MutexType;
        LoggerManager();
        Logger::ptr getLogger(const std::string &name);
        Logger::ptr getRoot() { return m_root; }

        void init();

        std::string toYamlString();

    private:
        std::map<std::string, Logger::ptr> m_loggers;
        Logger::ptr m_root;
        MutexType m_mutex;
    };

    typedef sylar::Singleton<LoggerManager> LoggerMgr;

} // namespace sylar