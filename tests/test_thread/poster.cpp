module;
#include "arc/macros.hpp"

#if !ARC_IMPORT_STD
#include <algorithm>
#include <atomic>
#include <bit>
#include <condition_variable>
#include <format>
#include <functional>
#include <future>
#include <iostream>
#include <memory>
#include <mutex>
#include <print>
#include <stdexcept>
#include <string>
#include <thread>
#include <typeindex>
#include <vector>
#endif

// #include <signal.h>
module arc.tests.thread.poster;

import arc;

namespace arc::tests::thread {

struct Thread
{
    static constexpr std::size_t Unassigned = -1;

    static bool hasMainThread() { return MainThread != nullptr; }
    static bool isMainThread() { return CurrentId == ThreadEnvironment::MainThreadId and MainThread == getCurrentThreadIdPtr(); }

    static void setMainThread()
    {
        if (hasMainThread())
            throw std::runtime_error("A thread has already been assigned as the main");
        CurrentId = ThreadEnvironment::MainThreadId;
        MainThread = getCurrentThreadIdPtr();
    }

    static void resetMainThread()
    {
        if (not isMainThread())
            throw std::runtime_error("Resetting main thread from thread other than the main thread");
        CurrentId = Unassigned;
        MainThread = nullptr;
    }

    static std::size_t getId()
    {
        return CurrentId;
    }

    static void setId(std::size_t threadId)
    {
        if (threadId == ThreadEnvironment::MainThreadId)
            return setMainThread();
        if (threadId == ThreadEnvironment::DynamicThreadId || threadId == ThreadEnvironment::AnyThreadId)
            throw std::runtime_error(std::format("Thread cannot be set to id {}", threadId));
        if (CurrentId != Unassigned)
            throw std::runtime_error(
                std::format("Thread has already been assigned ID {} while trying to assign ID {}", CurrentId, threadId));
        CurrentId = threadId;
    }

    static void* getCurrentThreadIdPtr() { return &CurrentId; }

private:
    inline static thread_local std::size_t CurrentId = Unassigned;
    inline static void* MainThread = nullptr;
};

struct Scheduler::ThreadContext
{
    explicit ThreadContext(Scheduler* scheduler, std::size_t threadId)
        : scheduler(scheduler), threadId(threadId)
    {
        Thread::setId(threadId);
    }

    bool postTask(auto&& task)
    {
        if (scheduler->isExclusiveMode()) [[unlikely]]
        {
            std::println("Scheduler is in exclusive mode, cannot post task to thread {}", threadId);
            return false;
        }
        {
            auto lk = std::lock_guard(mtx);
            if (stopOnEmpty) [[unlikely]]
            {
                std::println("Thread {} is stopping, cannot post task", threadId);
                return false;
            }
            if (auto const state = scheduler->setBusy(threadId); not state.isBusy())
            {
                std::println("Scheduler is in exclusive mode, cannot post task to thread {}", threadId);
                return false;
            }
            tasks.push_back(ARC_FWD(task));
        }
        cv.notify_one();
        return true;
    }

    std::size_t getThreadId() const { return threadId; }

    void stop()
    {
        stopOnEmpty = true;
        cv.notify_one();
    }

    void assertCurrentThread()
    {
        if (threadId != Thread::getId()) [[unlikely]]
        {
            throw std::runtime_error(std::format("Incorrect thread id {}, expected {}", Thread::getId(), threadId));
        }
    }

    bool isCurrentThread() const
    {
        return threadIdPtr == Thread::getCurrentThreadIdPtr() or scheduler->isExclusiveMode();
    }

    ~ThreadContext()
    {
        std::println("Thread {} finished", threadId);
    }

private:
    friend Scheduler;

    void run()
    {
        State state = scheduler->setBusy(threadId);
        if (not state.isBusy()) [[unlikely]]
        {
            auto lk = std::unique_lock(mtx);
            pauseLoop(lk, state);
        }

        for (Function<void()> task; true; )
        {
            {
                std::unique_lock lk(mtx);

                if (tasks.empty()) [[unlikely]]
                {
                    state = scheduler->setIdle(threadId);
                    // Enter pause loop to ensure any exclusive tasks are run before potentially stopping
                    pauseLoop(lk, state);
                    if (stopOnEmpty) [[unlikely]]
                    {
                        std::println("Thread {} stopping on empty", threadId);
                        break;
                    }
                    // continue;
                }

                task = std::move(tasks.front());
                tasks.pop_front();
            }
            task();
        }
    }

    ARC_COLD
    void pauseLoop(std::unique_lock<std::mutex>& lk, State& state)
    {
        do
        {
            if (state.isExclusiveThread(threadId))
            {
                lk.unlock();

                // TODO: Is this really true? What about other threads not owned by the scheduler?
                // Since we are the only thread running, we can don't need to use a mutex to read exclusiveTasks
                while (not scheduler->exclusiveTasks.empty())
                {
                    auto exclusiveTask = scheduler->exclusiveTasks.pop_front_value();
                    try
                    {
                        exclusiveTask();
                    }
                    catch (...)
                    {
                        scheduler->setIdle(threadId);
                        throw;
                    }
                }

                state = scheduler->setIdle(threadId);
                if (not state.isIdle()) [[unlikely]]
                    throw TerminateSchedulerThreadException(std::format("Unable to set thread {} to idle from exclusive", threadId));

                lk.lock();
            }

            // Wait until there is work to do, or we are stopping
            // TODO: This should wake when there is an exclusive task to run
            cv.wait(lk, [this] { return not tasks.empty() or stopOnEmpty; });

            if (tasks.empty())
                state = scheduler->state.load(std::memory_order_acquire);
            else
                state = scheduler->setBusy(threadId);
        } while (state.exclusive);
    }

    Scheduler* scheduler;
    std::size_t const threadId;
    void* const threadIdPtr = Thread::getCurrentThreadIdPtr();
    std::mutex mtx;
    std::condition_variable cv;
    CircularBuffer<Function<void()>> tasks{std::numeric_limits<std::size_t>::max()};
    bool stopOnEmpty = false;
};

auto Scheduler::setIdle(std::size_t const threadId) -> State
{
    auto current  = state.load(std::memory_order_relaxed);
    while (true)
    {
        auto next = current;
        if (next.exclusive)
        {
            if (not next.isExclusiveThread(threadId)) [[unlikely]]
                throw TerminateSchedulerThreadException(std::format("Trying to set non-exclusive thread {} to idle in exclusive mode", threadId));
            next.busyThreads = 0;
            next.exclusive = false;
        }
        else
        {
            if (std::popcount(next.busyThreads) == 1)
                next.exclusive = true;
            else
                next.busyThreads &= ~(std::uint64_t(1) << threadId);
        }

        if (state.compare_exchange_strong(current, next, std::memory_order_acq_rel))
            return next;
    }
}

auto Scheduler::setBusy(std::size_t threadId) -> State
{
    auto current  = state.load(std::memory_order_relaxed);
    while (true)
    {
        if (current.exclusive)
            return current;
        auto next = current;
        next.busyThreads |= (std::uint64_t(1) << threadId);

        if (state.compare_exchange_strong(current, next, std::memory_order_acq_rel))
            return next;
    }
}

bool Scheduler::isExclusiveMode() const
{
    return state.load(std::memory_order_relaxed).exclusive;
}

bool postTask(std::weak_ptr<Scheduler::ThreadContext> h, Function<void()> f)
{
    if (auto ctx = h.lock()) [[likely]]
        return ctx->postTask(std::move(f));

    return false;
}

Scheduler::Scheduler()
{
    mainThreadContext = std::make_shared<ThreadContext>(this, 0);
    threadContexts.push_back(mainThreadContext);
    // signal(SIGINT, [](int)
    // {
    //     std::puts("SIGINT detected, stopping all threads...");
    //     if (auto tc = Scheduler::get().lock())
    //         tc->stopAll();
    // });
}

void Scheduler::resetMainThread()
{
    Thread::resetMainThread();
}

bool Scheduler::isCurrentThread(std::size_t threadId) const
{
    if (auto c = getThread(threadId).lock())
        return c->isCurrentThread();
    return isExclusiveMode();
}

std::string Scheduler::currentThreadDetails()
{
    return std::format("ID: {}", Thread::getId());
}

auto Scheduler::addThread(std::weak_ptr<ThreadContext> previous) -> std::weak_ptr<ThreadContext>
{
    if (sealed.load(std::memory_order_acquire))
        throw std::runtime_error("Cannot add threads after some have started");
    auto lk = std::lock_guard(mtx);
    if (auto p = previous.lock())
    {
        threadContexts.emplace_back(previous);
        return previous;
    }
    auto threadId = threadContexts.size();
    std::promise<std::weak_ptr<ThreadContext>> newTc;
    auto thread = std::jthread{
        [&]()
        {
            std::shared_ptr<ThreadContext> threadContext;
            try
            {
                threadContext = std::make_shared<ThreadContext>(this, threadId);
                threadContexts.emplace_back(threadContext);
                newTc.set_value(threadContext);
            }
            catch (...)
            {
                return newTc.set_exception(std::current_exception());
            }

            // Wait until threads have been set up before starting any work
            sealed.wait(false, std::memory_order_acquire);

            while (true)
            {
                try
                {
                    threadContext->run();
                    break;
                }
                catch(TerminateSchedulerThreadException& e)
                {
                    std::println("Thread {} terminating on request: {}", threadContext->threadId, e.what());
                    threadContext.reset();
                    break;
                }
                catch(StopSchedulerException& e)
                {
                    std::println("Thread {} stopping all threads on request: {}", threadContext->threadId, e.what());
                    stopAll();
                    // Allow thread to exit cleanly
                    continue;
                }
                catch(std::exception& e)
                {
                    std::println("Unhandled exception in thread {}: {}", threadContext->threadId, e.what());
                }
                catch(...)
                {
                    std::println("Unhandled unknown exception in thread {}", threadContext->threadId);
                }
            }
        }
    };

    auto result = newTc.get_future().get();
    threads.emplace_back(std::move(thread));
    return result;
}

auto Scheduler::getThread(std::size_t threadId) const -> std::weak_ptr<ThreadContext>
{
    if (not sealed.load(std::memory_order_acquire)) [[unlikely]]
        throw std::runtime_error("Cannot getThread before threads have started");
    return threadContexts.at(threadId);
}

void Scheduler::startThreads()
{
    sealed.store(true, std::memory_order_release);
    sealed.notify_all();
}

void Scheduler::stopAll()
{
    auto lk = std::lock_guard(mtx);
    for (auto& tc : threadContexts)
        if (auto c = tc.lock())
            c->stop();
    startThreads();
}

bool Scheduler::postTask(std::size_t threadId, Function<void()> task)
{
    return thread::postTask(getThread(threadId), std::move(task));
}

// TODO get this to work when all threads are idle
bool Scheduler::postExclusiveTask(Function<void()> task)
{
    if (auto const s = state.load(std::memory_order_relaxed);
        s.isExclusiveThread(Thread::getId())) [[unlikely]]
    {
        task();
        return true;
    }
    std::lock_guard lg(mtx);
    exclusiveTasks.push_back(std::move(task));
    return true;
}

bool Scheduler::postExclusiveTask(arc::TypeId ti, Function<void()> task)
{
    if (auto const s = state.load(std::memory_order_relaxed);
        s.isExclusiveThread(Thread::getId())) [[unlikely]]
    {
        if (not std::ranges::contains(exclusiveTaskTags, ti))
            task();
        return true;
    }
    std::lock_guard lg(mtx);
    if (not std::ranges::contains(exclusiveTaskTags, ti))
    {
        exclusiveTaskTags.push_back(ti);
        exclusiveTasks.emplace_back(
            [this, ti, task = std::move(task)]() mutable
            {
                auto const clear = arc::Defer([this, ti]{ std::erase(exclusiveTaskTags, ti); });
                task();
            });
    }
    return true;
}

void Scheduler::run()
{
    if (not Thread::isMainThread())
        throw std::runtime_error("run must be run from the main thread");
    startThreads();
    if (not threadContexts.empty())
    {
        mainThreadContext->run();
        threads.clear();
        // Set state to exclusive mode as all threads are now stopped
        state.store({
            .exclusive = true,
            .busyThreads = 0,
        }, std::memory_order_release);
        threadContexts.clear();
    }
}

}
