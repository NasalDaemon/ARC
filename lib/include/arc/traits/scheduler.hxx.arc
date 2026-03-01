// To be implemented using C++26 std::execution, until then, this is the contract
trait arc::Scheduler
{
    isCurrentThread(std::size_t threadId) const -> std::convertible_to<bool> auto
    currentThreadDetails() const -> std::convertible_to<std::string> decltype(auto)

    // If true then it is guaranteed that no other scheduler threads are running a task apart from the current thread
    inExclusiveMode() const -> bool

    // Post a task to a thread that is to be run in exclusive mode
    template<IsRootCluster Cluster, StatelessInvocable<Cluster&> Task>
    postExclusiveTask(Cluster& cluster, Task&& task) const -> std::convertible_to<bool> decltype(auto)

    // As above, but do not queue duplicate entries with the same tag type
    template<arc::IsStateless Tag, IsRootCluster Cluster, StatelessInvocable<Cluster&> Task>
    postExclusiveTask(Tag tag, Cluster& cluster, Task&& task) const -> std::convertible_to<bool> decltype(auto)

    template<std::invocable Task>
    postTask(std::size_t threadId, Task&& task) const -> std::convertible_to<bool> decltype(auto)

    run()
    stop()
}
