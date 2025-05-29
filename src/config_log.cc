#include "config_log.h"
#include "log.h"
#include "config.h"

namespace sylar
{
    struct LogAppenderDefine
    {
        int type = 0; // 1: File, 2: STDOUT
        LogLevel::Level level = LogLevel::UNKNOWN;
        std::string filename;
        std::string format;

        bool operator==(const LogAppenderDefine &other) const
        {
            return type == other.type && level == other.level && format == other.format && filename == other.filename;
        }
    };

    struct LogDefine
    {
        std::string name;
        LogLevel::Level level = LogLevel::UNKNOWN;
        std::string format;
        std::vector<LogAppenderDefine> appenders;

        bool operator==(const LogDefine &other) const
        {
            return name == other.name && level == other.level && format == other.format && appenders == other.appenders;
        }
        bool operator<(const LogDefine &other) const
        {
            return name < other.name;
        }

        bool isValid() const
        {
            return !name.empty();
        }
    };

    ConfigVar<std::set<LogDefine>>::ptr g_log_defines = Config::Lookup("logs", std::set<LogDefine>(), "logs config");

    struct LogIniter
    {
        LogIniter()
        {
            g_log_defines->addListener(0xF1E231, [](const std::set<LogDefine> &old_val, const std::set<LogDefine> &new_val)
                                       {
                LOG_INFO(LOG_ROOT()) << "logger config changed";
                for (auto& i : new_val) {
                    Logger::ptr logger;
                    auto it = old_val.find(i);
                    if (it != old_val.end() && (i == *it)) {
                        continue;
                    } 
                    logger = LOG_NAME(i.name);
                    logger->setLevel(i.level);
                    if (!i.format.empty()) {
                        logger->setFormatter(i.format);
                    }
                    logger->clearAppenders();
                    for (auto& a : i.appenders) {
                        LogAppender::ptr ap;
                        if (a.type == 1) {
                            ap.reset(new FileLogAppender(a.filename));
                        } else if (a.type == 2) {
                            ap.reset(new StdoutLogAppender);
                        } else {
                            continue;
                        }
                        ap->setLevel(a.level);
                        if (!a.format.empty()) {
                            LogFormatter::ptr fmt(new LogFormatter(a.format));
                            if (!fmt->isError()) {
                                ap->setFormatter(fmt);
                            }
                            else {
                                std::cout << "log.name=" << i.name << " appender type=" << a.type
                                      << " formatter=" << a.format << " is invalid" << std::endl;
                            }
                        }

                        logger->addAppender(ap);
                    }
                } 

                for (auto & i : old_val) {
                    auto it = new_val.find(i);
                    if (it == new_val.end()) {
                        // remove logger
                        auto logger = LOG_NAME(i.name);
                        // set a very high threashold
                        logger->setLevel((LogLevel::Level)0);
                        logger->clearAppenders();
                    }
                } });
        }
    };

    static LogIniter __log_init;

    template <>
    class LexicalCast<std::string, LogDefine>
    {
    public:
        LogDefine operator()(const std::string &v)
        {
            LogDefine log;
            YAML::Node node = YAML::Load(v);

            if (!node["name"].IsDefined())
            {
                std::cout << "log config error: name is null, " << node << std::endl;
                throw std::logic_error("log config name is null.");
            }

            log.name = node["name"].as<std::string>();
            log.level = LogLevel::FromString(node["level"].IsDefined() ? node["level"].as<std::string>() : "");
            if (node["formatter"].IsDefined())
            {
                log.format = node["formatter"].as<std::string>();
            }

            if (node["appenders"].IsDefined())
            {
                for (size_t i = 0; i < node["appenders"].size(); ++i)
                {
                    auto a = node["appenders"][i];
                    LogAppenderDefine ap;
                    if (!a["type"].IsDefined())
                    {
                        std::cout << "log appender error: type is null, " << a << std::endl;
                        continue;
                    }
                    std::string type = a["type"].as<std::string>();
                    if (type == "FileLogAppender")
                    {
                        if (!a["file"].IsDefined())
                        {
                            std::cout << "log config error: filename not found " << a << std::endl;
                            continue;
                        }
                        ap.type = 1;
                        ap.filename = a["file"].as<std::string>();
                    }
                    else if (type == "StdoutLogAppender")
                    {
                        ap.type = 2;
                    }
                    else
                    {
                        std::cout << "log config error: appender type unknown " << a << std::endl;
                    }

                    if (a["formatter"].IsDefined())
                    {
                        ap.format = a["formatter"].as<std::string>();
                    }
                    log.appenders.push_back(ap);
                }
            }

            return log;
        }
    };

    // template specification for serialization for LogDefine
    template <>
    class LexicalCast<LogDefine, std::string>
    {
    public:
        std::string operator()(const LogDefine &v)
        {
            YAML::Node node;

            node["name"] = v.name;
            node["level"] = LogLevel::ToString(v.level);
            if (!v.format.empty())
            {
                node["format"] = v.format;
            }
            for (auto &appender : v.appenders)
            {
                YAML::Node node_appender;
                if (appender.type == 1)
                {
                    node_appender["type"] = "FileLogAppender";
                    node_appender["filename"] = appender.filename;
                }
                else if (appender.type == 2)
                {
                    node_appender["type"] = "StdoutLogAppender";
                }
                if (appender.level != LogLevel::UNKNOWN)
                {
                    node_appender["level"] = LogLevel::ToString(appender.level);
                }
                if (!appender.format.empty())
                {
                    node_appender["format"] = appender.format;
                }
                node["appenders"].push_back(node_appender);
            }
            std::stringstream ss;
            ss << node;

            return ss.str();
        }
    };

    void init_log_from_yml()
    {
    }
}