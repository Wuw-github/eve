#pragma once

#include "scheduler.h"

namespace sylar
{

    class IOManager : public Scheduler
    {

    public:
        typedef std::shared_ptr<IOManager> ptr;
        typedef RWMutex RWMutexType;

        enum Event
        {
            NONE = 0x0,
            READ = 0x1,
            WRITE = 0x2,
        };

    private:
        struct FdContext
        {
            typedef MutexType Mutex;
            struct EventContext
            {
                Scheduler *scheduler = nullptr;
                Fiber::ptr fiber;
                std::function<void()> cb;
            };

        public:
            // file descriptor
            int fd = 0;
            EventContext read;
            EventContext write;

            Event m_event = NONE;
            MutexType m_mutex;
        };

    public:
        IOManager(size_t threads = 1, bool use_caller = true, const std::string &name = "");
        ~IOManager();

        int addEvent(int fd, Event event, std::function<void()> cb = nullptr);
        bool delEvent(int fd, Event event);
        bool cancelEvent(int fd, Event event);

        bool cancelAll(int fd);

        static IOManager *GetThis();

    protected:
        void tickle() override;

        bool stopping() override;

        void idle() override;

        void contextResize(size_t size);

    private:
        int m_epfd = 0;
        // for pipe
        int m_tickleFds[2];

        std::atomic<size_t> m_pendingEventCount{0};
        RWMutexType m_mutex;
        std::vector<FdContext *> m_fdContexts;
    };
}