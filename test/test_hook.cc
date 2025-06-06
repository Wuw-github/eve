#include "hook.h"
#include "iomanager.h"
#include "fiber.h"
#include "log.h"

using namespace sylar;

Logger::ptr g_logger = LOG_ROOT();
void test_sleep()
{
    IOManager iom(1);
    iom.schedule([]()
                 { sleep(2); 
                    LOG_INFO(g_logger) << "sleep 2"; });

    iom.schedule([]()
                 { sleep(3); 
                    LOG_INFO(g_logger) << "sleep 3"; });

    LOG_INFO(g_logger) << "test_sleep";
}

int main()
{
    test_sleep();
}