#pragma once

#include <string.h>
#include <assert.h>
#include "util.h"
#include "log.h"

#define _ASSERT(x)                                                                \
    if (!(x))                                                                     \
    {                                                                             \
        LOG_ERROR(LOG_NAME("system")) << " ASSERTION: " #x                        \
                                      << "\nbacktrace: \n"                        \
                                      << sylar::BacktraceToString(20, 2, "    "); \
        assert(x);                                                                \
    }

#define _ASSERT2(x, w)                                                            \
    if (!(x))                                                                     \
    {                                                                             \
        LOG_ERROR(LOG_NAME("system")) << " ASSERTION: " #x                        \
                                      << "\n"                                     \
                                      << w                                        \
                                      << "\nbacktrace: \n"                        \
                                      << sylar::BacktraceToString(20, 2, "    "); \
        assert(x);                                                                \
    }