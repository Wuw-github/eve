#include "thread.h"
#include "log.h"
#include "config.h"

using namespace sylar;

Logger::ptr g_logger = LOG_ROOT();

int count = 0;
Mutex s_mutex;

void fun1()
{

    LOG_INFO(LOG_ROOT()) << "thread_id " << Thread::GetName()
                         << " this.name: " << Thread::GetThis()->getName()
                         << " id: " << GetThreadId()
                         << " this.id: " << Thread::GetThis()->getId();

    for (int i = 0; i < 1000; ++i)
    {
        Mutex::Lock lock(s_mutex);
        count++;
    }
}

void fun2()
{
    while (true)
    {
        LOG_INFO(g_logger) << "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx";
    }
}

void fun3()
{
    while (true)
    {
        LOG_INFO(g_logger) << "=================================================";
    }
}

int main(int argc, char **argv)
{
    using namespace sylar;
    LOG_INFO(LOG_ROOT()) << "thread test begin";
    YAML::Node root = YAML::LoadFile("/home/wu/Documents/sylar/bin/conf/log2.yml");
    Config::LoadFromYaml(root);
    LoggerManager &mgr = LoggerMgr::GetInstance();
    std::cout << "======================" << std::endl;
    std::cout << mgr.toYamlString() << std::endl;
    std::vector<Thread::ptr> threads;
    for (int i = 0; i < 2; ++i)
    {
        Thread::ptr thr1(new Thread(&fun2, "name_" + std::to_string(i * 2)));
        Thread::ptr thr2(new Thread(&fun3, "name_" + std::to_string(i * 2 + 1)));
        threads.push_back(thr1);
        threads.push_back(thr2);
    }

    for (int i = 0; i < threads.size(); ++i)
    {
        threads[i]->join();
    }
    LOG_INFO(LOG_ROOT()) << "thread test finish";

    LOG_INFO(g_logger) << "count: " << count;
    return 0;
}