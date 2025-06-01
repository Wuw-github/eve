#include "macro.h"
#include <assert.h>
#include "util.h"
#include "log.h"
#include "config.h"

sylar::Logger::ptr g_logger = LOG_ROOT();

using namespace sylar;
void test()
{
    // LOG_INFO(g_logger) << std::endl
    //                    << BacktraceToString(10, 0, "   ");

    _ASSERT2(0 == 1, "abcdef ");
}

int main(int argc, char **argv)
{
    YAML::Node root = YAML::LoadFile("/home/wu/Documents/sylar/bin/conf/log.yml");
    Config::LoadFromYaml(root);
    test();
}