#include "scheduler.h"
#include "log.h"
#include "config.h"

using namespace sylar;

Logger::ptr g_logger = LOG_ROOT();

void test_fiber()
{
    static int count = 5;
    LOG_INFO(g_logger) << "test in fiber";
    // sleep(2);
    if (--count > 0)
    {
        Scheduler::GetThis()->schedule(&test_fiber);
    }
}

int main(int argc, char *argv[])
{
    Scheduler sc(3, true, "test");

    sc.start();
    sc.schedule(&test_fiber);
    sc.stop();

    LOG_INFO(g_logger) << "over from main function.";
    std::cout.flush();
}