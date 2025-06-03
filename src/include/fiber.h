#pragma once

#include <ucontext.h>
#include <functional>
#include <memory>
#include "thread.h"
#include "mutex.h"

namespace sylar
{
    class Scheduler;

    class Fiber : public std::enable_shared_from_this<Fiber>
    {
        friend class Scheduler;

    public:
        typedef std::shared_ptr<Fiber> ptr;
        enum State
        {
            INIT,
            HOLD,
            EXEC,
            TERM,
            READY,
            EXCEPT
        };

    public:
        Fiber(std::function<void()> cb, size_t stacksize = 0, bool use_caller = false);
        ~Fiber();

        // reset fiber function and state, only in INIT or TERM
        void reset(std::function<void()> cb);

        // switch to current fiber
        void swapIn();

        void call();

        void back();

        // switch out and stay in background
        void swapOut();

        uint64_t getId() const { return m_id; }

        State getState() const { return m_state; }

    public:
        // set current fiber
        static void SetThis(Fiber *f);
        // return current fiber, if fiber does not exist for current thread, main fiber will becreated and return
        static Fiber::ptr GetThis();

        // fiber switch to background and set to READY
        static void YieldToReady();

        // fiber switch to background and set to HOLD
        static void YieldToHold();

        // total num of fibers
        static uint64_t TotalFibers();

        static void MainFunc();

        static void CallerMainFunc();

        static uint64_t GetFiberId();

    private:
        // default constructor only used by main thread fiber
        Fiber();

        uint64_t m_id{0};
        State m_state{INIT};
        uint32_t m_stacksize{0};

        ucontext_t m_ctx;
        void *m_stack = nullptr;
        std::function<void()> m_cb;
    };
}