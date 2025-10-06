#ifndef INCLUDE_ARC_ARGS_HPP
#define INCLUDE_ARC_ARGS_HPP

#include "arc/macros.hpp"

#if !ARC_IMPORT_STD
#include <tuple>
#endif

namespace arc {

ARC_MODULE_EXPORT
template<class Tag_, class... Ts>
struct Args
{
    using Tag = Tag_;

    constexpr explicit Args(Ts&&... args) : args(ARC_FWD(args)...) {}
    // Args should always be passed around as a temporary reference, no need for relocation
    Args(Args&&) = delete;
    Args(Args const&) = delete;

    template<class T>
    T make() const
    {
        return std::make_from_tuple<T>(std::move(args));
    }

    template<class T>
    auto&& get() const
    {
        return std::get<T&&>(args);
    }

private:
    std::tuple<Ts&&...> args;
};

ARC_MODULE_EXPORT
template<class Tag, class... Ts>
constexpr Args<Tag, Ts...> args(Ts&&... args)
{
    return Args<Tag, Ts...>{ARC_FWD(args)...};
}

namespace detail {
    template<class Tag, class... Ts>
    constexpr void isArgs(Args<Tag, Ts...> const&);
}

ARC_MODULE_EXPORT
template<class T>
concept IsArgs = requires(T t) { detail::isArgs(t); };

ARC_MODULE_EXPORT
template<class T, class Tag>
concept IsArgsOf = IsArgs<T> and std::same_as<typename std::remove_reference_t<T>::Tag, Tag>;

}

#endif // INCLUDE_ARC_ARGS_HPP
