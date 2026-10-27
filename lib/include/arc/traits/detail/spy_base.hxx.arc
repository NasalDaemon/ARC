trait arc::detail::SpyBase
{
    // Const, as calls are intercepted from const contexts too
    template<class Method, class... Args, std::invocable<Args...> Impl>
    intercept(Method method, Impl const& impl, Args&&... args) const

    // Ability to switch spy on/off at runtime. Returns true if state is as requested.
    enable() -> bool { return false; }
    disable() -> bool { return false; }
}
