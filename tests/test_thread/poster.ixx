module;
#include "arc/macros.hpp"

#if !ARC_IMPORT_STD
#include <atomic>
#include <cstddef>
#include <format>
#include <future>
#include <memory>
#include <mutex>
#include <queue>
#include <thread>
#include <type_traits>
#include <typeindex>
#include <utility>
#include <vector>
#endif
export module arc.tests.thread.poster;

import arc;

namespace arc::tests::thread {

export struct Scheduler
{
    struct ThreadContext;

private:
    std::shared_ptr<ThreadContext> mainThreadContext{};
    std::vector<std::weak_ptr<ThreadContext>> threadContexts{};
    std::vector<std::jthread> threads{};
    std::mutex mtx{};
    std::atomic_bool sealed = false;

    struct State
    {
        enum class Mode : std::uint32_t
        {
            Busy, Idle, Exclusive,
        };

        // Initially one busy thread (the main thread)
        std::uint16_t busyThreads = 1;
        std::uint16_t exclusiveThread = 0;
        Mode mode = Mode::Busy;

        bool isBusy() const { return mode == Mode::Busy; }
        bool isIdle() const { return mode == Mode::Idle; }
        bool isExclusive() const { return mode == Mode::Exclusive; }
    };

    std::atomic<State> state;
    static_assert(decltype(state)::is_always_lock_free);

    std::queue<Function<void()>> exclusiveTasks;
    std::vector<std::type_index> exclusiveTaskTags;

    static void resetMainThread();

    State setBusy();
    State setIdle(std::size_t threadId);

public:
    Scheduler();
    Scheduler(Scheduler const&) = delete;
    Scheduler(Scheduler&&) = delete;

    bool isCurrentThread(std::size_t threadId) const;
    static std::string currentThreadDetails();
    bool isExclusiveMode() const;

    std::weak_ptr<ThreadContext> addThread(std::weak_ptr<ThreadContext> previous = {});
    std::weak_ptr<ThreadContext> getThread(std::size_t id) const;
    void startThreads();

    void run();

    [[nodiscard]] bool postTask(std::size_t threadId, Function<void()> task);
    [[nodiscard]] bool postExclusiveTask(Function<void()> task);
    [[nodiscard]] bool postExclusiveTask(std::type_index tag, Function<void()> task);

    void stopAll();

    ~Scheduler()
    {
        auto g = arc::Defer([] { resetMainThread(); });
        stopAll();
        run();
    }
};

export [[nodiscard]] bool postTask(std::weak_ptr<Scheduler::ThreadContext> h, Function<void()> f);

struct FireAndForget : arc::key::ThreadPost<FireAndForget>
{
    template<std::size_t CurrentThreadId, std::size_t RequiredThreadId, class Task>
    [[nodiscard]] static constexpr bool post(auto scheduler, std::size_t requiredThreadId, Task&& task)
    {
        if constexpr (CurrentThreadId == ThreadEnvironment::DynamicThreadId
                   or RequiredThreadId == ThreadEnvironment::DynamicThreadId
                   or CurrentThreadId != RequiredThreadId)
        {
            if (not scheduler.isCurrentThread(requiredThreadId))
            {
                return scheduler.postTask(requiredThreadId, ARC_FWD(task));
            }
        }

        // Already known to be on correct thread
        task();
        return true;
    }
};

export inline constexpr FireAndForget fireAndForget{};

struct Future : arc::key::ThreadPost<Future>
{
    template<std::size_t CurrentThreadId, std::size_t RequiredThreadId, std::invocable Task>
    [[nodiscard]] static constexpr auto post(auto scheduler, std::size_t requiredThreadId, Task&& task)
        -> std::future<std::invoke_result_t<std::remove_cvref_t<Task>>>
    {
        using R = std::invoke_result_t<std::remove_cvref_t<Task>>;
        auto package = std::packaged_task<R()>(ARC_FWD(task));
        auto future = package.get_future();
        if constexpr (CurrentThreadId == ThreadEnvironment::DynamicThreadId
                   or RequiredThreadId == ThreadEnvironment::DynamicThreadId
                   or CurrentThreadId != RequiredThreadId)
        {
            if (not scheduler.isCurrentThread(requiredThreadId))
            {
                if (not scheduler.postTask(requiredThreadId, std::move(package))) [[unlikely]]
                {
                    std::promise<R> promise;
                    promise.set_exception(std::make_exception_ptr(
                        std::runtime_error(std::format("Unable to post task to thread {}", requiredThreadId))));
                    future = promise.get_future();
                }
                return future;
            }
        }

        // Already known to be on correct thread, so execute immediately
        package();
        return future;
    }
};

export inline constexpr Future future{};

} // namespace arc::tests::thread
