#ifndef INCLUDE_ARC_ALIAS_HPP
#define INCLUDE_ARC_ALIAS_HPP

#include "arc/detail/compress.hpp"

#include "arc/macros.hpp"

#if !ARC_IMPORT_STD
#include <memory>
#include <tuple>
#include <utility>
#endif

namespace arc {

ARC_MODULE_EXPORT
template<class T, class... Key>
struct Alias final
{
    static_assert(sizeof...(Key) == 0);

    using Impl = T;
    using Interface = T;

    explicit constexpr Alias(Impl& impl, Key...) : impl(std::addressof(impl)) {}

    template<class Self>
    ARC_INLINE constexpr auto& get(this Self&& self) { return std::forward_like<Self&>(*self.impl); }
    ARC_INLINE constexpr auto* operator->(this auto&& self) { return std::addressof(self.get()); }

private:
    Impl* impl;
};

template<class T, class Key, class... Keys>
struct Alias<T, Key, Keys...> final
{
    using Impl = T;
    using Interface = Alias;
    using Traits = Impl::Traits;

    constexpr Alias(auto& alias, Key const& key, Keys const&... keys)
        : alias(alias), key(key), keys(keys...)
    {}

    constexpr auto& get(this auto&& self) { return self; }
    constexpr auto* operator->(this auto&& self) { return std::addressof(self); }

    template<class... Ts>
    ARC_INLINE constexpr auto impl(this auto&& self, Ts&&... args)
        -> decltype(self.alias->implWithKey(self.key, self.keys, ARC_FWD(args)...))
    {
        return self.alias->implWithKey(self.key, self.keys, ARC_FWD(args)...);
    }

    ARC_INLINE constexpr decltype(auto) visit(this auto&& self, auto&& visitor)
    {
        return self.alias->visit(ARC_FWD(visitor));
    }

private:
    Alias<Impl> alias;
    [[no_unique_address]] Key key;
    [[no_unique_address]] std::tuple<Keys...> keys;
};

template<class T, class... Key>
Alias(T&, Key...) -> Alias<T, Key...>;

ARC_MODULE_EXPORT
constexpr auto makeAlias(auto& impl, auto const&... keys)
{
    return Alias(detail::compressImpl(impl), keys...);
}

} // namespace arc


#endif // INCLUDE_ARC_ALIAS_HPP
