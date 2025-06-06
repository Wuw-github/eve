#pragma once

#include <memory>
#include <vector>
#include "thread.h"
#include "iomanager.h"
#include "hook.h"
#include "singleton.h"

namespace sylar
{
    class FdCtx : public std::enable_shared_from_this<FdCtx>
    {
    public:
        typedef std::shared_ptr<FdCtx> ptr;
        typedef RWMutex RWMutexType;

        FdCtx(int fd);
        ~FdCtx();

        bool init();
        bool isInit() const { return m_isInit; };

        bool isSocket() const { return m_isSocket; }
        bool isNonblock() const { return m_sysNonblock; }
        int fd() const { return m_fd; }

        bool isClosed() const { return m_isClosed; }
        void close();

        void setUserNonBlock(bool v) { m_userNonblock = v; }
        bool getUserNonBlock() const { return m_userNonblock; }

        void setSysNonBlock(bool v) { m_sysNonblock = v; }
        bool getSysNonBlock() const { return m_sysNonblock; }

        void setTimeout(int type, uint64_t v);

        uint64_t getTimeout(int type) const;

    private:
        bool m_isInit{false};
        bool m_isSocket{false};
        bool m_sysNonblock{false};
        bool m_userNonblock{false};
        bool m_isClosed{false};
        int m_fd;

        uint64_t m_recvTimeout;
        uint64_t m_sendTimeout;
    };

    class FdManager
    {
    public:
        typedef RWMutex RWMutexType;

        FdManager();

        FdCtx::ptr get(int fd, bool auto_create = false);

        void del(int fd);

    private:
        RWMutexType m_mutex;
        std::vector<FdCtx::ptr> m_fdCtxs;
    };

    typedef Singleton<FdManager> FdMgr;

}