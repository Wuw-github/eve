#include "fd_manager.h"
#include <sys/types.h>
#include <sys/stat.h>

namespace sylar
{
    FdCtx::FdCtx(int fd)
        : m_fd(fd)
    {
        init();
    }

    bool FdCtx::init()
    {
        if (m_isInit)
        {
            return true;
        }
        m_recvTimeout = -1;
        m_sendTimeout = -1;

        struct stat fd_stat;
        // check fd state
        if (-1 == fstat(m_fd, &fd_stat))
        {
            m_isInit = false;
            m_isSocket = false;
        }
        else
        {
            m_isInit = true;
            m_isSocket = S_ISSOCK(fd_stat.st_mode);
        }

        if (m_isSocket)
        {
            int flags = fcntl_f(m_fd, F_GETFL, 0);
            if (!(flags & O_NONBLOCK))
            {
                fcntl_f(m_fd, F_SETFL, flags | O_NONBLOCK);
            }
            m_sysNonblock = true;
        }
        else
        {
            m_sysNonblock = false;
        }

        m_userNonblock = false;
        m_isClosed = false;

        return m_isInit;
    }

    FdCtx::~FdCtx()
    {
    }

    void FdCtx::setTimeout(int type, uint64_t v)
    {
        if (type == SO_RCVTIMEO)
        {
            m_recvTimeout = v;
        }
        else
        {
            m_sendTimeout = v;
        }
    }

    uint64_t FdCtx::getTimeout(int type) const
    {
        if (type == SO_RCVTIMEO)
        {
            return m_recvTimeout;
        }
        else
        {
            return m_sendTimeout;
        }
    }

    FdManager::FdManager()
    {
        m_fdCtxs.resize(64);
    }

    FdCtx::ptr FdManager::get(int fd, bool auto_create)
    {
        if (fd < 0)
        {
            return nullptr;
        }
        RWMutexType::ReadLock lock(m_mutex);
        if ((int)m_fdCtxs.size() <= fd)
        {
            if (!auto_create)
            {
                return nullptr;
            }
        }
        else
        {
            if (m_fdCtxs[fd] || !auto_create)
            {
                return m_fdCtxs[fd];
            }
        }

        lock.unlock();
        RWMutexType::WriteLock lock2(m_mutex);
        FdCtx::ptr ctx(new FdCtx(fd));
        if (fd >= (int)m_fdCtxs.size())
        {
            m_fdCtxs.resize(fd * 1.5);
        }
        m_fdCtxs[fd] = ctx;
        return ctx;
    }

    void FdManager::del(int fd)
    {
        RWMutexType::WriteLock lock(m_mutex);
        if ((int)m_fdCtxs.size() <= fd)
        {
            return;
        }
        m_fdCtxs[fd].reset();
    }
}