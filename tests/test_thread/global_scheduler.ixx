module;
#include "arc/macros.hpp"

#if !ARC_IMPORT_STD
#include <cstddef>
#include <concepts>
#include <functional>
#include <string>
#include <typeindex>
#endif
export module arc.tests.thread.global_scheduler;

import arc.tests.thread.poster;
import arc;

namespace arc::tests::thread {

struct GlobalScheduler_ : arc::Node
{
    using Traits = arc::Traits<GlobalScheduler_, arc::trait::Scheduler>;

    struct Types
    {
        using ThreadIdType = std::size_t;
    };

    std::string impl(arc::trait::Scheduler::currentThreadDetails) const
    {
        return Scheduler::currentThreadDetails();
    }

    bool impl(arc::trait::Scheduler::isCurrentThread, std::size_t threadId) const
    {
        return scheduler.isCurrentThread(threadId);
    }

    bool impl(arc::trait::Scheduler::inExclusiveMode) const
    {
        return scheduler.isExclusiveMode();
    }

    template<IsRootCluster Cluster, StatelessInvocable<Cluster&> Task>
    bool impl(arc::trait::Scheduler::postExclusiveTask, Cluster& cluster, Task&& task) const
    {
        return scheduler.postExclusiveTask(
            [task = ARC_FWD(task), &cluster]() mutable
            {
                task(cluster);
            });
    }

    template<IsStateless Tag, IsRootCluster Cluster, StatelessInvocable<Cluster&> Task>
    bool impl(arc::trait::Scheduler::postExclusiveTask, Tag, Cluster& cluster, Task&& task) const
    {
        return scheduler.postExclusiveTask(
            std::type_index{typeid(Tag)},
            [task = ARC_FWD(task), &cluster]() mutable
            {
                task(cluster);
            });
    }

    bool impl(arc::trait::Scheduler::postTask, std::size_t threadId, auto&& task) const
    {
        return scheduler.postTask(threadId, ARC_FWD(task));
    }

    void impl(arc::trait::Scheduler::run)
    {
        scheduler.run();
    }

    void impl(arc::trait::Scheduler::stop)
    {
        scheduler.stopAll();
    }

    explicit GlobalScheduler_(std::invocable<Scheduler&> auto f)
    {
        f(scheduler);
        scheduler.startThreads();
    }

    Scheduler mutable scheduler;
};

export
using GlobalScheduler = arc::AnyThread<GlobalScheduler_>;

}
