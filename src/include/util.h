#pragma once

#include <pthread.h>
#include <sys/types.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <stdint.h>

namespace sylar
{

    pid_t GetThreadId();

    uint32_t GetFiberId();

}