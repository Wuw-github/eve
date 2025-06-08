#include "hook.h"
#include "iomanager.h"
#include "fiber.h"
#include "log.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <string.h>
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

void test_sock()
{
    int sock = socket(AF_INET, SOCK_STREAM, 0);

    sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(80);
    // addr.sin_addr.s_addr = INADDR_ANY;
    inet_pton(AF_INET, "103.235.46.115", &addr.sin_addr.s_addr);
    int rt = connect(sock, (const sockaddr *)&addr, sizeof(addr));

    LOG_INFO(g_logger) << "connect rt=" << rt << " errno=" << errno;

    if (rt)
    {
        return;
    }

    const char data[] = "GET / HTTP/1.0\r\n\r\n";
    rt = send(sock, data, sizeof(data), 0);
    LOG_INFO(g_logger) << "send rt=" << rt << " errno=" << errno;
    if (rt <= 0)
    {
        return;
    }

    std::string buff;
    buff.resize(4096);

    rt = recv(sock, &buff[0], 4096, 0);
    LOG_INFO(g_logger) << "recv rt=" << rt << " errno=" << errno;
    if (rt <= 0)
    {
        return;
    }
    buff.resize(rt);
    LOG_INFO(g_logger) << buff.c_str();
}

int main()
{
    // test_sleep();
    // test_sock();
    sylar::IOManager iom;
    iom.schedule(test_sock);
}