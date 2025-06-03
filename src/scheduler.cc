#include "scheduler.h"
#include "log.h"
#include "macro.h"

namespace sylar
{
    static Logger::ptr g_logger = LOG_NAME("system");

    static thread_local Scheduler *t_scheduler = nullptr;
    static thread_local Fiber *t_fiber = nullptr;

    Scheduler::Scheduler(size_t threadCount, bool use_caller, const std::string &name)
        : m_name(name)
    {
        _ASSERT(threadCount > 0);
        if (use_caller)
        {
            // initialize a main fiber
            Fiber::GetThis();
            --threadCount;

            // cannot have more than one scheduler
            _ASSERT(Scheduler::GetThis() == nullptr);
            t_scheduler = this;

            // root fiber is not equals to the main fiber of the thread
            m_rootFiber.reset(new Fiber(std::bind(&Scheduler::run, this), 0, true));
            Thread::SetName(m_name);

            // main fiber should be the fiber that running scheduler
            t_fiber = m_rootFiber.get();
            m_rootThread = GetThreadId();
            m_threadIds.push_back(m_rootThread);
        }
        else
        {
            m_rootThread = -1;
        }
        m_threadCount = threadCount;
    }

    Scheduler::~Scheduler()
    {
        _ASSERT(m_stopping);
        if (GetThis() == this)
        {
            t_scheduler = nullptr;
        }
    }

    Scheduler *Scheduler::GetThis()
    {
        return t_scheduler;
    }

    Fiber *Scheduler::GetMainFiber()
    {
        return t_fiber;
    }

    void Scheduler::start()
    {
        MutexType::Lock lock(m_mutex);
        if (!m_stopping)
        {
            return;
        }

        m_stopping = false;
        _ASSERT(m_threads.empty());

        m_threads.resize(m_threadCount);
        for (size_t i = 0; i < m_threadCount; ++i)
        {
            m_threads[i].reset(new Thread(std::bind(&Scheduler::run, this),
                                          m_name + "_" + std::to_string(i)));
            m_threadIds.push_back(m_threads[i]->getId());
        }
        // lock.unlock();
        // if (m_rootFiber)
        // {
        //     m_rootFiber->call();
        //     // m_rootFiber->swapIn();
        // }
    }

    void Scheduler::stop()
    {
        m_autoStop = true;
        if (m_rootFiber && m_threadCount == 0 && (m_rootFiber->getState() == Fiber::TERM || m_rootFiber->getState() == Fiber::INIT))
        {
            // if only one thread exists in the thread pool, return from this path
            LOG_INFO(g_logger) << this << " stopped";
            m_stopping = true;

            if (stopping())
            {
                return;
            }
        }

        // bool exit_on_this_fiber = false;
        if (m_rootThread != -1)
        {
            _ASSERT(GetThis() == this);
        }
        else
        {
            _ASSERT(GetThis() != this);
        }

        m_stopping = true;
        for (size_t i = 0; i < m_threadCount; ++i)
        {
            tickle();
        }

        if (m_rootFiber)
        {
            tickle();
        }

        if (m_rootFiber)
        {
            if (!stopping())
            {
                m_rootFiber->call();
            }
        }

        std::vector<Thread::ptr> thrs;
        {
            MutexType::Lock lock(m_mutex);
            thrs.swap(m_threads);
        }
        for (auto &i : thrs)
        {
            i->join();
        }
    }

    void Scheduler::setThis()
    {
        t_scheduler = this;
    }

    void Scheduler::run()
    {
        LOG_DEBUG(g_logger) << m_name << " run";
        setThis();
        if (GetThreadId() != m_rootThread)
        {
            t_fiber = Fiber::GetThis().get();
        }

        // if all tasks are finished, idle_fiber kicks in to do the idle operation
        Fiber::ptr idle_fiber(new Fiber(std::bind(&Scheduler::idle, this)));
        // cb_fiber is used to handle the cb function if it is not wrapped in existing fiber
        Fiber::ptr cb_fiber;

        FiberAndThread ft;
        while (true)
        {
            ft.reset();
            bool tickle_me = false;
            bool is_active = false;
            {
                MutexType::Lock lock(m_mutex);
                auto it = m_fibers.begin();
                while (it != m_fibers.end())
                {
                    // if a fiber task is assigned to a specific thread, skip it
                    if (it->thread != -1 && it->thread != GetThreadId())
                    {
                        ++it;
                        // tell other threads to work on this task
                        tickle_me = true;
                        continue;
                    }
                    // for a given task, at least a fiber instance or a call back fun should be provided
                    _ASSERT(it->fiber || it->cb);
                    if (it->fiber && it->fiber->getState() == Fiber::EXEC)
                    {
                        ++it;
                        continue;
                    }
                    ++m_activeThreads;
                    is_active = true;
                    ft = *it;
                    // tickle_me = true;
                    m_fibers.erase(it);
                    break;
                }
                tickle_me |= it != m_fibers.end();
            }

            if (tickle_me)
            {
                tickle();
            }

            if (ft.fiber && ft.fiber->getState() != Fiber::TERM && ft.fiber->getState() != Fiber::EXCEPT)
            {
                // start the task
                ft.fiber->swapIn();
                --m_activeThreads;

                if (ft.fiber->getState() == Fiber::READY)
                {
                    // put back to the task queue
                    schedule(ft.fiber);
                }
                else if (ft.fiber->getState() != Fiber::TERM && ft.fiber->getState() != Fiber::EXCEPT)
                {
                    // what to do with HOLD next?
                    ft.fiber->m_state = Fiber::HOLD;
                }
                ft.reset();
            }
            else if (ft.cb)
            {
                if (cb_fiber)
                {
                    // push call back function to cb_fiber
                    cb_fiber->reset(ft.cb);
                }
                else
                {
                    cb_fiber.reset(new Fiber(ft.cb));
                }
                ft.reset();
                // start task
                LOG_INFO(g_logger) << "Thread start to run cb";
                cb_fiber->swapIn();
                LOG_INFO(g_logger) << "Thread finish to run cb";
                --m_activeThreads;

                if (cb_fiber->getState() == Fiber::READY)
                {
                    schedule(cb_fiber);
                    cb_fiber.reset();
                }
                else if (cb_fiber->getState() == Fiber::TERM || cb_fiber->getState() == Fiber::EXCEPT)
                {
                    cb_fiber->reset(nullptr);
                }
                else
                {
                    cb_fiber->m_state = Fiber::HOLD;
                    cb_fiber.reset();
                }
            }
            else
            {
                if (is_active)
                {
                    --m_activeThreads;
                    continue;
                }
                if (idle_fiber->getState() == Fiber::TERM)
                {
                    LOG_INFO(g_logger) << "idle fiber terminated.";
                    break;
                }

                ++m_idleThreads;
                idle_fiber->swapIn();
                --m_idleThreads;
                if (idle_fiber->getState() != Fiber::TERM && idle_fiber->getState() != Fiber::EXCEPT)
                {
                    // next iteration will continue to execute hold logic
                    idle_fiber->m_state = Fiber::HOLD;
                }
            }
        }
    }

    void Scheduler::tickle()
    {
        LOG_INFO(g_logger) << "tickle";
    }

    void Scheduler::idle()
    {
        LOG_INFO(g_logger) << "idle";

        while (!stopping())
        {
            Fiber::YieldToHold();
            // LOG_INFO(g_logger) << "idle go out";
        }
    }

    bool Scheduler::stopping()
    {
        MutexType::Lock lock(m_mutex);
        return m_autoStop && m_stopping && m_fibers.empty() && m_activeThreads == 0;
    }
}