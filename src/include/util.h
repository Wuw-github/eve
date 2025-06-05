#pragma once

#include <pthread.h>
#include <sys/types.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <stdint.h>
#include <vector>
#include <string>

namespace sylar
{

    pid_t GetThreadId();

    uint32_t GetFiberId();

    void Backtrace(std::vector<std::string> &bt, int size, int skip = 1);

    std::string BacktraceToString(int size, int skip = 2, const std::string &prefix = "");

    // time
    uint64_t GetCurrentMS();
    uint64_t GetCurrentUS();

} // namespace sylar
