#ifndef INCLUDE_ARC_TRAITS_SPY_HPP
#define INCLUDE_ARC_TRAITS_SPY_HPP

#include "arc/macros.hpp"

#pragma push_macro("ARC_MODULE_EXPORT")
// Do not export SpyBase, internal only
#undef ARC_MODULE_EXPORT
#define ARC_MODULE_EXPORT
#include "arc/traits/detail/spy_base.hxx"
#pragma pop_macro("ARC_MODULE_EXPORT")

namespace arc::inline trait {

ARC_MODULE_EXPORT
template<class Trait>
struct SpyOnly : detail::SpyBase
{
    struct Meta : detail::SpyBase::Meta
    {
        struct ImplQualified : detail::SpyBase::Meta::Methods
        {
            struct SpyOnly : detail::SpyBase::Meta::MethodTags
            {
                using Impl = detail::SpyBase::Meta::Methods;
            };
            static void impl() = delete;
        };
    };

    static void canProvide(SpyOnly);
    static SpyOnly expects();
};

ARC_MODULE_EXPORT
template<class Trait>
inline constexpr SpyOnly<Trait> spyOnly{};

ARC_MODULE_EXPORT
struct Spy : detail::SpyBase
{
    struct Meta : detail::SpyBase::Meta
    {
        struct ImplQualified : detail::SpyBase::Meta::Methods
        {
            struct Spy : detail::SpyBase::Meta::MethodTags
            {
                using Impl = detail::SpyBase::Meta::Methods;
            };
            static void impl() = delete;
        };
    };

    template<class Trait>
    static void canProvide(SpyOnly<Trait>);
    static void canProvide(Spy);

    static Spy expects();
} inline constexpr spy{};

}

#endif // INCLUDE_ARC_TRAITS_SPY_HPP
