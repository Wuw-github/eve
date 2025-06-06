#pragma once
#include "thread.h"
#include "fiber.h"
#include "mutex.h"
#include <memory>
#include <vector>
#include <list>
namespace sylar
{

    // scheduler --> threads --> fiber
    // 1. thread pool
    // 2. schedule fiber execution to thread
    class Scheduler
    {
    public:
        typedef std::shared_ptr<Scheduler> ptr;
        typedef Mutex MutexType;

        Scheduler(size_t threadCount = 1, bool use_caller = true, const std::string &name = "");
        virtual ~Scheduler();

        const std::string &getName() const { return m_name; }

        // get the schduler instance
        static Scheduler *GetThis();

        static Fiber *GetMainFiber();

        void start();

        void stop();

        template <class FiberOrCb>
        void schedule(FiberOrCb fc, int thread = -1)
        {
            bool need_tickle = false;
            {
                MutexType::Lock lock(m_mutex);
                need_tickle = scheduleNoLock(fc, thread);
            }
            if (need_tickle)
            {
                tickle();
            }
        }

        template <class InputIterator>
        void schedule(InputIterator begin, InputIterator end)
        {
            bool need_tickle = false;
            {
                MutexType::Lock lock(m_mutex);
                while (begin != end)
                {
                    need_tickle |= scheduleNoLock(&*begin, -1); // swap the content
                    ++begin;
                }
            }
            if (need_tickle)
            {
                tickle();
            }
        }

    protected:
        virtual void tickle();

        void run();

        virtual bool stopping();

        virtual void idle();

        void setThis();

        bool hasIdleThreads() { return m_idleThreads > 0; }

    private:
        template <class FiberOrCb>
        bool scheduleNoLock(FiberOrCb fc, int thread)
        {
            bool need_tickle = m_fibers.empty();
            FiberAndThread ft(fc, thread);
            if (ft.fiber || ft.cb)
            {
                m_fibers.push_back(ft);
            }
            return need_tickle;
        }

    private:
        struct FiberAndThread
        {
            Fiber::ptr fiber;
            std::function<void()> cb;
            int thread;

            FiberAndThread(Fiber::ptr fiber, int thr)
                : fiber(fiber), thread(thr) {};
            FiberAndThread(Fiber::ptr *f, int thr)
                : thread(thr)
            {
                fiber.swap(*f);
            }

            FiberAndThread(std::function<void()> cb, int thr)
                : cb(cb), thread(thr) {}

            FiberAndThread(std::function<void()> *c, int thr)
                : thread(thr)
            {
                cb.swap(*c);
            }

            FiberAndThread() : thread(-1) {}

            void reset()
            {
                fiber = nullptr;
                cb = nullptr;
                thread = -1;
            }
        };

    protected:
        std::vector<int> m_threadIds{};
        size_t m_threadCount{0};
        std::atomic<size_t> m_activeThreads{0};
        std::atomic<size_t> m_idleThreads{0};

        bool m_stopping{true};
        bool m_autoStop{false};

        int m_rootThread{0};

    private:
        MutexType m_mutex;
        std::vector<Thread::ptr> m_threads;
        std::list<FiberAndThread> m_fibers;
        std::string m_name;
        Fiber::ptr m_rootFiber;
    };

}