module;
#include "arc/macros.hpp"

#if !ARC_IMPORT_STD
#include <atomic>
#include <bit>
#include <cstddef>
#include <format>
#include <future>
#include <limits>
#include <memory>
#include <mutex>
#include <thread>
#include <type_traits>
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
        static constexpr std::size_t MaxThreads = 8 * sizeof(std::size_t) - 1;

        bool isBusy() const { return not exclusive and busyThreads != 0; }
        bool isIdle() const { return not exclusive and busyThreads == 0; }
        bool isExclusiveThread(std::size_t threadId) const { return exclusive and threadId <= MaxThreads and busyThreads == (std::size_t(1) << threadId); }

        // Initially one busy thread (the main thread)
        bool exclusive : 1 = false;
        std::size_t busyThreads : MaxThreads = 1;
    };

    std::atomic<State> state;
    static_assert(decltype(state)::is_always_lock_free);

    CircularBuffer<Function<void()>> exclusiveTasks{std::numeric_limits<std::size_t>::max()};
    std::vector<arc::TypeId> exclusiveTaskTags;

    static void resetMainThread();

    State setBusy(std::size_t threadId);
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
    [[nodiscard]] bool postExclusiveTask(arc::TypeId tag, Function<void()> task);

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
