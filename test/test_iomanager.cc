#include "iomanager.h"
#include "log.h"
#include "config.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <arpa/inet.h>
#include <netinet/in.h>

using namespace sylar;

Logger::ptr g_logger = LOG_NAME("root");

void test_fiber()
{
    LOG_INFO(g_logger) << "test_fiber";

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    fcntl(sock, F_SETFL, O_NONBLOCK);

    sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(80);
    // addr.sin_addr.s_addr = INADDR_ANY;
    inet_pton(AF_INET, "103.235.46.115", &addr.sin_addr.s_addr);

    // iomanager.addEvent(sock, IOManager::Event::READ, []()
    //    { LOG_INFO(g_logger) << "read connected"; });

    if (!connect(sock, (const sockaddr *)&addr, sizeof(addr)))
    {
    }
    else if (errno == EINPROGRESS)
    {
        LOG_INFO(g_logger) << "add event errno=" << errno << " " << strerror(errno);
        IOManager::GetThis()->addEvent(sock, sylar::IOManager::READ, []()
                                       { LOG_INFO(g_logger) << "read callback"; });
        IOManager::GetThis()->addEvent(sock, sylar::IOManager::WRITE, [sock]()
                                       {
                LOG_INFO(g_logger) << "write callback";
                //close(sock);
                bool res = IOManager::GetThis()->cancelEvent(sock, sylar::IOManager::READ);
                LOG_INFO(g_logger) << "cancel res=" << res;
                close(sock); });
    }
    else
    {
        LOG_INFO(g_logger) << "else " << errno << " " << strerror(errno);
    }
}

void test1()
{
    IOManager iomanager(3, false, "iomanager");
    iomanager.schedule(&test_fiber);
}

Timer::ptr s_timer;
void test_timer()
{
    IOManager iom(2, true, "msg");

    s_timer = iom.addTimer(1000, []()
                           { LOG_INFO(g_logger) << "hello from test timer"; 
                    static int i = 0;
                if (++i == 5) {
                    s_timer->reset(2000, true);
                } }, true);
}

int main(int argc, char *argv[])
{
    LOG_INFO(g_logger) << "start testing";
    test_timer();

    LOG_INFO(g_logger) << "exit from main";
}