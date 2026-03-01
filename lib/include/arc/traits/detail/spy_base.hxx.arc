trait arc::detail::SpyBase
{
    template<class Method, class... Args, std::invocable<Args...> Impl>
    intercept(Method method, Impl const& impl, Args&&... args)
}
