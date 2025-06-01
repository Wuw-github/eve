#include "fiber.h"
#include "log.h"
#include "config.h"

using namespace sylar;

Logger::ptr g_logger = LOG_NAME("root");

void run_in_fiber()
{
    LOG_INFO(g_logger) << "run in fiber begin";
    Fiber::YieldToHold();

    // throw std::runtime_error("test");
    LOG_INFO(g_logger) << "run in fiber finish";
    Fiber::YieldToHold();
}

int main()
{
    // YAML::Node root = YAML::LoadFile("/home/wu/Documents/sylar/bin/conf/log.yml");
    // Config::LoadFromYaml(root);

    {
        Fiber::GetThis();
        LOG_INFO(g_logger) << "main begin";
        Fiber::ptr fiber(new Fiber(run_in_fiber));
        fiber->swapIn();
        LOG_INFO(g_logger) << "main after swapIn";
        fiber->swapIn();
        LOG_INFO(g_logger) << "main after fiber finish";
        fiber->swapIn();
    }
    LOG_INFO(g_logger) << "main finish";
    std::cout.flush();
    return 0;
}