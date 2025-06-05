#include "iomanager.h"
#include "macro.h"
#include "log.h"

#include <errno.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
namespace sylar
{

    static Logger::ptr g_logger = LOG_NAME("system");

    IOManager::FdContext::EventContext &IOManager::FdContext::getContext(Event event)
    {
        switch (event)
        {
        case IOManager::Event::READ:
            return read;
        case IOManager::Event::WRITE:
            return write;
        defaule:
            _ASSERT2(false, "getContext");
        }
        throw std::invalid_argument("getContext invalid event");
    }

    void IOManager::FdContext::resetContext(EventContext &ctx)
    {
        ctx.scheduler = nullptr;
        ctx.fiber.reset();
        ctx.cb = nullptr;
    }

    void IOManager::FdContext::triggerEvent(Event event)
    {
        _ASSERT(events & event);
        events = (Event)(events & ~event);
        EventContext &ctx = getContext(event);

        if (ctx.cb)
        {
            ctx.scheduler->schedule(&ctx.cb);
        }
        else
        {
            ctx.scheduler->schedule(&ctx.fiber);
        }
        ctx.scheduler = nullptr;
    }

    IOManager::IOManager(size_t threads, bool use_caller, const std::string &name)
        : Scheduler(threads, use_caller, name)
    {
        m_epfd = epoll_create(5000);
        _ASSERT2(m_epfd > 0, "epoll_creation error");

        int rt = pipe(m_tickleFds);
        _ASSERT2(rt == 0, "pipe error");

        epoll_event event;
        memset(&event, 0, sizeof(event));

        event.events = EPOLLIN | EPOLLET;
        event.data.fd = m_tickleFds[0];

        rt = fcntl(m_tickleFds[0], F_SETFL, O_NONBLOCK);
        _ASSERT2(rt == 0, "fcntl error");

        rt = epoll_ctl(m_epfd, EPOLL_CTL_ADD, m_tickleFds[0], &event);
        _ASSERT2(rt == 0, "epoll_ctl error");

        contextResize(32);

        start();
    }

    IOManager::~IOManager()
    {
        stop();
        close(m_epfd);
        close(m_tickleFds[0]);
        close(m_tickleFds[1]);
        for (size_t i = 0; i < m_fdContexts.size(); ++i)
        {
            if (m_fdContexts[i])
            {
                delete m_fdContexts[i];
            }
        }
    }

    void IOManager::contextResize(size_t size)
    {
        m_fdContexts.resize(size);
        for (size_t i = 0; i < m_fdContexts.size(); ++i)
        {
            if (!m_fdContexts[i])
            {
                m_fdContexts[i] = new FdContext;
                m_fdContexts[i]->fd = i;
            }
        }
    }

    int IOManager::addEvent(int fd, Event event, std::function<void()> cb)
    {
        FdContext *fd_ctx = nullptr;
        RWMutexType::ReadLock lock(m_mutex);
        if ((int)m_fdContexts.size() > fd)
        {
            fd_ctx = m_fdContexts[fd];
            lock.unlock();
        }
        else
        {
            lock.unlock();
            RWMutexType::WriteLock lock2(m_mutex);
            contextResize(fd * 1.5);
            fd_ctx = m_fdContexts[fd];
        }

        FdContext::MutexType::Lock lockCtx(fd_ctx->m_mutex);
        if (fd_ctx->events & event)
        {
            LOG_ERROR(g_logger) << "addEvent assert fd=" << fd
                                << " event=" << (EPOLL_EVENTS)event
                                << " fd_ctx.event=" << (EPOLL_EVENTS)fd_ctx->events;
            _ASSERT(!(fd_ctx->events & event));
        }

        int op = fd_ctx->events ? EPOLL_CTL_MOD : EPOLL_CTL_ADD;
        epoll_event epevent;
        epevent.events = EPOLLET | fd_ctx->events | event;
        epevent.data.ptr = fd_ctx;

        int rt = epoll_ctl(m_epfd, op, fd, &epevent);
        if (rt != 0)
        {
            LOG_ERROR(g_logger) << "epoll_ctl(" << m_epfd << ", "
                                << op << ", " << fd << ", " << (EPOLL_EVENTS)epevent.events << "):"
                                << rt << " (" << errno << ") (" << strerror(errno) << ") fd_ctx->events="
                                << (EPOLL_EVENTS)fd_ctx->events;
            return -1;
        }

        ++m_pendingEventCount;
        fd_ctx->events = (Event)(fd_ctx->events | event);
        FdContext::EventContext &event_ctx = fd_ctx->getContext(event);

        // new event should have empty properties
        _ASSERT(!event_ctx.scheduler && !event_ctx.fiber && !event_ctx.cb);

        event_ctx.scheduler = Scheduler::GetThis();
        if (cb)
        {
            event_ctx.cb.swap(cb);
        }
        else
        {
            event_ctx.fiber = Fiber::GetThis();
            _ASSERT2(event_ctx.fiber->getState() == Fiber::EXEC, "state=" << event_ctx.fiber->getState());
        }

        return 0;
    }

    bool IOManager::delEvent(int fd, Event event)
    {
        RWMutexType::ReadLock lock(m_mutex);
        if ((int)m_fdContexts.size() > fd)
        {
            return false;
        }
        FdContext *fd_ctx = m_fdContexts[fd];
        lock.unlock();

        FdContext::MutexType::Lock lock2(fd_ctx->m_mutex);
        if (!(fd_ctx->events & event))
        {
            return false;
        }

        Event new_events = (Event)(fd_ctx->events & ~event);
        int op = new_events ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;

        epoll_event epevent;
        epevent.events = EPOLLET | new_events;
        epevent.data.ptr = fd_ctx;

        int rt = epoll_ctl(m_epfd, op, fd, &epevent);
        if (rt != 0)
        {
            LOG_ERROR(g_logger) << "epoll_ctl(" << m_epfd << ", "
                                << op << ", " << fd << ", " << (EPOLL_EVENTS)epevent.events << "):"
                                << rt << " (" << errno << ") (" << strerror(errno) << ")";
            return false;
        }

        --m_pendingEventCount;
        fd_ctx->events = new_events;
        FdContext::EventContext &event_ctx = fd_ctx->getContext(event);
        fd_ctx->resetContext(event_ctx);
        return true;
    }
    bool IOManager::cancelEvent(int fd, Event event)
    {
        RWMutexType::ReadLock lock(m_mutex);
        if ((int)m_fdContexts.size() <= fd)
        {
            return false;
        }
        FdContext *fd_ctx = m_fdContexts[fd];
        lock.unlock();

        FdContext::MutexType::Lock lock2(fd_ctx->m_mutex);
        if (!(fd_ctx->events & event))
        {
            return false;
        }

        Event new_events = (Event)(fd_ctx->events & ~event);
        int op = new_events ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;

        epoll_event epevent;
        epevent.events = EPOLLET | new_events;
        epevent.data.ptr = fd_ctx;

        int rt = epoll_ctl(m_epfd, op, fd, &epevent);
        if (rt != 0)
        {
            LOG_ERROR(g_logger) << "epoll_ctl(" << m_epfd << ", "
                                << op << ", " << fd << ", " << (EPOLL_EVENTS)epevent.events << "):"
                                << rt << " (" << errno << ") (" << strerror(errno) << ")";
            return false;
        }

        fd_ctx->triggerEvent(event);
        --m_pendingEventCount;
        return true;
    }

    bool IOManager::cancelAll(int fd)
    {
        RWMutexType::ReadLock lock(m_mutex);
        if ((int)m_fdContexts.size() > fd)
        {
            return false;
        }
        FdContext *fd_ctx = m_fdContexts[fd];
        lock.unlock();

        FdContext::MutexType::Lock lock2(fd_ctx->m_mutex);
        if (!fd_ctx->events)
        {
            return false;
        }

        int op = EPOLL_CTL_DEL;

        epoll_event epevent;
        epevent.events = 0;
        epevent.data.ptr = fd_ctx;

        int rt = epoll_ctl(m_epfd, op, fd, &epevent);
        if (rt != 0)
        {
            LOG_ERROR(g_logger) << "epoll_ctl(" << m_epfd << ", "
                                << op << ", " << fd << ", " << (EPOLL_EVENTS)epevent.events << "):"
                                << rt << " (" << errno << ") (" << strerror(errno) << ")";
            return false;
        }

        if (fd_ctx->events & READ)
        {
            fd_ctx->triggerEvent(READ);
            --m_pendingEventCount;
        }
        if (fd_ctx->events & WRITE)
        {
            fd_ctx->triggerEvent(WRITE);
            --m_pendingEventCount;
        }

        _ASSERT(fd_ctx->events == 0);
        return true;
    }

    IOManager *IOManager::GetThis()
    {
        return dynamic_cast<IOManager *>(Scheduler::GetThis());
    }

    void IOManager::tickle()
    {
        if (!hasIdleThreads())
        {
            return;
        }
        // write to trigger event
        int rt = write(m_tickleFds[1], "T", 1);
        _ASSERT(rt == 1);
    }

    bool IOManager::stopping(uint64_t &timeout)
    {
        timeout = getNextTimer();
        return timeout == ~0ull && Scheduler::stopping() && m_pendingEventCount == 0;
    }

    bool IOManager::stopping()
    {
        uint64_t timeout = 0;
        return stopping(timeout);
    }

    void IOManager::idle()
    {
        epoll_event *events = new epoll_event[64]();
        std::shared_ptr<epoll_event> shared_events(events, [](epoll_event *p)
                                                   { delete[] p; });
        while (true)
        {
            if (stopping())
            {
                LOG_INFO(g_logger) << "name=" << getName() << " idle stopping exits.";
                break;
            }
            int rt = 0;
            do
            {
                static const uint64_t MAX_TIMEOUT = 3000;
                uint64_t next_timeout = std::min(MAX_TIMEOUT, getNextTimer());

                rt = epoll_wait(m_epfd, events, 64, (int)next_timeout);

                if (rt < 0 && errno == EINTR)
                {
                    continue;
                }
                break;
            } while (true);

            // execute the timer events
            std::vector<std::function<void()>> cbs;
            listExpiredCb(cbs);
            if (!cbs.empty())
            {
                // LOG_DEBUG(g_logger) << "on timer cbs.size=" << cbs.size();
                schedule(cbs.begin(), cbs.end());
                cbs.clear();
            }

            // check all the epoll events
            for (int i = 0; i < rt; i++)
            {
                epoll_event &event = events[i];

                // if data is from the pipe, it means thread is tickled, read the data and dumped
                if (event.data.fd == m_tickleFds[0])
                {
                    uint8_t dummy[256];
                    // use ET mode, so read all the data
                    while (read(m_tickleFds[0], dummy, sizeof(dummy)) > 0)
                        ;
                    continue;
                }

                FdContext *fd_ctx = (FdContext *)event.data.ptr;
                FdContext::MutexType::Lock lock(fd_ctx->m_mutex);
                if (event.events & (EPOLLERR | EPOLLHUP))
                {
                    event.events |= EPOLLIN | EPOLLOUT;
                }

                int real_events = NONE;
                if (event.events & EPOLLIN)
                {
                    real_events |= READ;
                }
                if (event.events & EPOLLOUT)
                {
                    real_events |= WRITE;
                }

                if (fd_ctx->events & real_events == NONE)
                {
                    continue;
                }

                int left_events = (fd_ctx->events & ~real_events);
                int op = left_events ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;

                event.events = EPOLLET | left_events;

                int rt2 = epoll_ctl(m_epfd, op, fd_ctx->fd, &event);
                if (rt2)
                {
                    LOG_ERROR(g_logger) << "epoll_ctl(" << m_epfd << ", "
                                        << op << ", " << fd_ctx->fd << ", " << (EPOLL_EVENTS)event.events << "):"
                                        << rt2 << " (" << errno << ") (" << strerror(errno) << ")";
                    continue;
                }

                if (real_events & READ)
                {
                    fd_ctx->triggerEvent(READ);
                    --m_pendingEventCount;
                }
                if (real_events & WRITE)
                {
                    fd_ctx->triggerEvent(WRITE);
                    --m_pendingEventCount;
                }
            }

            // Fiber::ptr cur = Fiber::GetThis();
            // auto raw_ptr = cur.get();
            // cur.reset();
            // raw_ptr->swapOut();
            Fiber::YieldToHold();
        }
    }

    void IOManager::onTimerInsertedAtFront()
    {
        tickle();
    }
}