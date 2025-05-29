#include "thread.h"
#include "log.h"

using namespace sylar;

Logger::ptr g_logger = LOG_ROOT();

int count = 0;
RWMutex s_mutex;

void fun1()
{

    LOG_INFO(LOG_ROOT()) << "thread_id " << Thread::GetName()
                         << " this.name: " << Thread::GetThis()->getName()
                         << " id: " << GetThreadId()
                         << " this.id: " << Thread::GetThis()->getId();

    for (int i = 0; i < 100000; ++i)
    {
        RWMutex::WriteLock lock(s_mutex);
        count++;
    }
}

void fun2()
{
}

int main(int argc, char **argv)
{
    using namespace sylar;
    LOG_INFO(LOG_ROOT()) << "thread test begin";
    std::vector<Thread::ptr> threads;
    for (int i = 0; i < 5; ++i)
    {
        Thread::ptr thr(new Thread(&fun1, "name_" + std::to_string(i)));
        threads.push_back(thr);
    }

    for (int i = 0; i < 5; ++i)
    {
        threads[i]->join();
    }
    LOG_INFO(LOG_ROOT()) << "thread test finish";

    LOG_INFO(g_logger) << "count: " << count;
    return 0;
}