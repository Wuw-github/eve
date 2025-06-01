#include "fiber.h"
#include "config.h"
#include "log.h"
#include "macro.h"
#include <atomic>

namespace sylar
{
    static Logger::ptr g_logger = LOG_NAME("system");

    static std::atomic<uint64_t> s_fiber_id{0};
    static std::atomic<uint64_t> s_fiber_count{0};

    // main thread fiber
    static thread_local Fiber *t_fiber = nullptr;

    // running fiber for current thread
    static thread_local Fiber::ptr t_threadFiber = nullptr;

    static ConfigVar<uint32_t>::ptr g_fiber_stack_size = Config::Lookup<uint32_t>("fiber.stack_size", 1024 * 1024, "fiber stack size");

    class MallocStackAllocator
    {
    public:
        static void *Alloc(size_t size)
        {
            return malloc(size);
        }

        static void Dealloc(void *vp, size_t sz)
        {
            free(vp);
        }
    };

    using StackAllocator = MallocStackAllocator;

    // default constructor only used by default fiber
    Fiber::Fiber()
    {
        m_state = EXEC;
        SetThis(this);

        if (getcontext(&m_ctx) != 0)
        {
            _ASSERT2(false, "getcontext");
        }
        m_id = s_fiber_id++;
        ++s_fiber_count;
        LOG_DEBUG(g_logger) << "Fiber::Fiber()";
    }

    Fiber::Fiber(std::function<void()> cb, size_t stacksize)
        : m_cb(std::move(cb))
    {
        m_id = s_fiber_id++;
        m_state = INIT;
        m_stacksize = stacksize ? stacksize : g_fiber_stack_size->getValue();

        m_stack = StackAllocator::Alloc(m_stacksize);
        if (getcontext(&m_ctx) != 0)
        {
            _ASSERT2(false, "getcontext");
        }
        m_ctx.uc_link = nullptr;
        m_ctx.uc_stack.ss_sp = m_stack;
        m_ctx.uc_stack.ss_size = m_stacksize;

        makecontext(&m_ctx, &Fiber::MainFunc, 0);
        s_fiber_count++;
        LOG_DEBUG(g_logger) << "Fiber::Fiber() id=" << m_id;
    }

    Fiber::~Fiber()
    {
        if (m_stack)
        {
            _ASSERT(m_state == TERM || m_state == INIT || m_state == EXCEPT);
            StackAllocator::Dealloc(m_stack, m_stacksize);
        }
        else
        {
            _ASSERT(!m_cb);
            _ASSERT(m_state == EXEC);

            Fiber *cur = t_fiber;
            if (cur == this)
            {
                SetThis(nullptr);
            }
        }
        --s_fiber_count;
        LOG_DEBUG(g_logger) << "Fiber::~Fiber id=" << m_id
                            << " total=" << s_fiber_count;
    }

    void Fiber::reset(std::function<void()> cb)
    {
        _ASSERT(m_stack);
        _ASSERT(m_state == TERM || m_state == INIT || m_state == EXCEPT);

        m_cb = cb;
        if (getcontext(&m_ctx) != 0)
        {
            _ASSERT2(false, "getcontext");
        }

        m_ctx.uc_link = nullptr;
        m_ctx.uc_stack.ss_sp = m_stack;
        m_ctx.uc_stack.ss_size = m_stacksize;

        makecontext(&m_ctx, &Fiber::MainFunc, 0);
        m_state = INIT;
    }

    void Fiber::swapIn()
    {
        SetThis(this);
        _ASSERT(m_state != EXEC);
        m_state = EXEC;
        if (swapcontext(&t_threadFiber->m_ctx, &m_ctx))
        {
            _ASSERT2(false, "swapcontext");
        }
    }

    void Fiber::swapOut()
    {
        SetThis(t_threadFiber.get());
        if (swapcontext(&m_ctx, &t_threadFiber->m_ctx))
        {
            _ASSERT2(false, "swapcontext");
        }
    }

    void Fiber::SetThis(Fiber *f)
    {
        t_fiber = f;
    }

    Fiber::ptr Fiber::GetThis()
    {
        if (t_fiber)
        {
            return t_fiber->shared_from_this();
        }
        Fiber::ptr main_fiber(new Fiber);
        _ASSERT(t_fiber == main_fiber.get());
        t_threadFiber = main_fiber;
        return t_fiber->shared_from_this();
    }

    void Fiber::YieldToReady()
    {
        Fiber::ptr cur = GetThis();
        cur->m_state = READY;
        cur->swapOut();
    }

    void Fiber::YieldToHold()
    {
        Fiber::ptr cur = GetThis();
        cur->m_state = HOLD;
        cur->swapOut();
    }

    uint64_t Fiber::TotalFibers()
    {
        return s_fiber_count;
    }

    void Fiber::MainFunc()
    {
        Fiber::ptr cur = Fiber::GetThis();
        _ASSERT(cur);
        try
        {
            cur->m_cb();
            cur->m_cb = nullptr;
            cur->m_state = TERM;
        }
        catch (const std::exception &e)
        {
            cur->m_state = EXCEPT;
            LOG_ERROR(g_logger) << "Fiber Except: " << e.what()
                                << " fiber_id=" << cur->m_id << std::endl
                                << sylar::BacktraceToString(20);
        }
        catch (...)
        {
            cur->m_state = EXCEPT;
            LOG_ERROR(g_logger) << "Fiber Except"
                                << " fiber_id=" << cur->m_id << std::endl
                                << sylar::BacktraceToString(20);
        }

        auto raw_ptr = cur.get();
        cur.reset();
        raw_ptr->swapOut();

        _ASSERT2(false, "never reach");
    }

    uint64_t Fiber::GetFiberId()
    {
        if (t_fiber)
        {
            return t_fiber->getId();
        }
        return 0;
    }
}