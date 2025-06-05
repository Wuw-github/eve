#pragma once

#include "scheduler.h"
#include "timer.h"

namespace sylar
{

    class IOManager : public Scheduler, public TimerManager
    {

    public:
        typedef std::shared_ptr<IOManager> ptr;
        typedef RWMutex RWMutexType;

        enum Event
        {
            NONE = 0x0,
            READ = 0x1,  // EPOLLIN
            WRITE = 0x4, // EPOLLOUT
        };

    private:
        struct FdContext
        {
            typedef Mutex MutexType;
            struct EventContext
            {
                Scheduler *scheduler = nullptr;
                Fiber::ptr fiber;
                std::function<void()> cb;
            };

        public:
            EventContext &getContext(Event event);
            void resetContext(EventContext &ctx);

            void triggerEvent(Event event);

        public:
            // file descriptor
            int fd = 0;
            EventContext read;
            EventContext write;

            Event events = NONE;
            MutexType m_mutex;
        };

    public:
        IOManager(size_t threads = 1, bool use_caller = true, const std::string &name = "");
        ~IOManager();

        // 0 - success, -1 - fail
        int addEvent(int fd, Event event, std::function<void()> cb = nullptr);
        bool delEvent(int fd, Event event);
        bool cancelEvent(int fd, Event event);

        bool cancelAll(int fd);

        static IOManager *GetThis();

    protected:
        void tickle() override;

        bool stopping() override;

        bool stopping(uint64_t &timeout);

        void idle() override;

        void contextResize(size_t size);

        void onTimerInsertedAtFront() override;

    private:
        int m_epfd = 0;
        // for pipe
        int m_tickleFds[2];

        std::atomic<size_t> m_pendingEventCount{0};
        RWMutexType m_mutex;
        std::vector<FdContext *> m_fdContexts;
    };
}