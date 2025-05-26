#include "log.h"
#include "config.h"

sylar::ConfigVar<int>::ptr g_int_value_config = sylar::Config::Lookup("system.port", (int)8080, "system port");

int main(int argc, char *argv[])
{

    using namespace sylar;

    SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << g_int_value_config->getValue();
    SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << g_int_value_config->toString();
}