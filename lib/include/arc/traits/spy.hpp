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
        struct Impl : detail::SpyBase::Meta::Impl
        {
            using SpyOnly = detail::SpyBase::Meta::Impl::SpyBase;
        private:
            using SpyBase = detail::SpyBase::Meta::Impl::SpyBase;
        };
        struct ImplQualified : detail::SpyBase::Meta::ImplQualified
        {
            using SpyOnly = detail::SpyBase::Meta::ImplQualified::SpyBase;
        private:
            using SpyBase = detail::SpyBase::Meta::ImplQualified::SpyBase;
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
        struct Impl : detail::SpyBase::Meta::Impl
        {
            using Spy = detail::SpyBase::Meta::Impl::SpyBase;
        private:
            using SpyBase = detail::SpyBase::Meta::Impl::SpyBase;
        };
        struct ImplQualified : detail::SpyBase::Meta::ImplQualified
        {
            using Spy = detail::SpyBase::Meta::ImplQualified::SpyBase;
        private:
            using SpyBase = detail::SpyBase::Meta::ImplQualified::SpyBase;
        };
    };

    template<class Trait>
    static void canProvide(SpyOnly<Trait>);
    static void canProvide(Spy);

    static Spy expects();
} inline constexpr spy{};

} // namespace arc::inline trait

namespace arc {

namespace detail {
    template<class T>
    inline constexpr bool isSpyTrait = false;
    template<class T>
    inline constexpr bool isSpyTrait<SpyOnly<T>> = true;
    template<>
    inline constexpr bool isSpyTrait<Spy> = true;
}

ARC_MODULE_EXPORT
template<class T>
concept IsSpyTrait = detail::isSpyTrait<T>;

} // namespace arc

#endif // INCLUDE_ARC_TRAITS_SPY_HPP
