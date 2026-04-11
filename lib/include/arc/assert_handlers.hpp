#ifndef INCLUDE_ARC_ASSERT_HANDLERS_HPP
#define INCLUDE_ARC_ASSERT_HANDLERS_HPP

#include "arc/macros.hpp"

#if !ARC_IMPORT_STD
#include <iostream>
#if ARC_STACKTRACE_ENABLED
#include <stacktrace>
#endif
#include <stdexcept>
#include <utility>
#endif

namespace arc {

ARC_MODULE_EXPORT
struct AssertHandlerBase
{
    static constexpr bool enabled = true;
};

ARC_MODULE_EXPORT
template<class T>
concept IsAssertHandler = std::derived_from<T, AssertHandlerBase> and std::invocable<T const, bool, const char*>;

ARC_MODULE_EXPORT
struct ContractAssertHandler : AssertHandlerBase
{
    #if __cpp_contracts >= 202502L
    ARC_INLINE constexpr void operator()(auto, bool value, const char* message) const
    {
        contract_assert(value);
    }
    #else
    ARC_INLINE constexpr void operator()(auto, bool, const char*) const
    {
        static_assert(false, "ContractAssertHandler: requires compiler support for C++20 contracts or later");
    }
    #endif
};

ARC_MODULE_EXPORT
struct IgnoreAssertHandler : AssertHandlerBase
{
    static constexpr bool enabled = false;
    ARC_INLINE constexpr void operator()(auto, bool, const char*) const
    {}
};

ARC_MODULE_EXPORT
struct AssumeAssertHandler : AssertHandlerBase
{
    ARC_INLINE constexpr void operator()(auto, bool value, const char*) const
    {
        if (not value)
            std::unreachable();
    }
};

ARC_MODULE_EXPORT
struct ContractViolation : std::logic_error
{
    using std::logic_error::logic_error;
};

namespace detail {
    template<class Exception>
    [[noreturn]] constexpr void throwException(auto&&... args)
    {
        throw Exception(ARC_FWD(args)...);
    }

    template<bool Abort>
    struct ThrowAssertHandler : AssertHandlerBase
    {
        ARC_INLINE constexpr void operator()(auto, bool value, const char* message) const
        {
            if (not value) [[unlikely]]
                handleViolation(message);
        }

    private:
        [[noreturn]] ARC_COLD static constexpr void handleViolation(const char* message) noexcept(Abort)
        {
            std::cerr << message << "\n";
            #if ARC_STACKTRACE_ENABLED
            if constexpr (Abort)
                std::cerr << "Stack trace:\n" << std::stacktrace::current(3) << "\n" << std::flush;
            #endif
            throwException<ContractViolation>(message);
        }
    };
}

ARC_MODULE_EXPORT
using AbortAssertHandler = detail::ThrowAssertHandler<true>;
ARC_MODULE_EXPORT
using ThrowAssertHandler = detail::ThrowAssertHandler<false>;

ARC_MODULE_EXPORT
#ifdef NDEBUG
using DefaultAssertHandler = IgnoreAssertHandler;
#else
using DefaultAssertHandler = AbortAssertHandler;
#endif

} // namespace arc

#endif // INCLUDE_ARC_ASSERT_HANDLERS_HPP
