#ifndef INCLUDE_ARC_VIRTUAL_FWD_HPP
#define INCLUDE_ARC_VIRTUAL_FWD_HPP

#include "arc/detail/compress.hpp"
#include "arc/context_fwd.hpp"
#include "arc/macros.hpp"
#include "arc/node_fwd.hpp"

#if !ARC_IMPORT_STD
#include <concepts>
#include <memory>
#endif

namespace arc {

ARC_MODULE_EXPORT
struct IDestructible
{
    virtual ~IDestructible() = default;
};

ARC_MODULE_EXPORT
struct [[nodiscard, maybe_unused]] KeepAlive
{
    explicit constexpr KeepAlive(std::unique_ptr<IDestructible> m) : m(std::move(m)) {}
    KeepAlive() = default;
    KeepAlive(KeepAlive&&) = default;
    KeepAlive& operator=(KeepAlive&&) = default;

private:
    std::unique_ptr<IDestructible> m;
};

namespace detail {
    struct INodeBase;
    struct IsVirtualContextTag{};

    template<IsNodeWrapper T, class Context>
    auto toVirtualNodeImpl() -> T::template Node<CompressContext<Context>>;
    template<IsNode T, class>
    auto toVirtualNodeImpl() -> T;

    template<IsNodeHandle T, class Context>
    using ToVirtualNodeImpl = decltype(toVirtualNodeImpl<T, Context>());

    template<class>
    inline constexpr bool injectVirtualHost = false;
}

ARC_MODULE_EXPORT
template<class T>
concept IsInterface = std::derived_from<T, detail::INodeBase> and requires {
    typename T::Traits;
};

ARC_MODULE_EXPORT
template<class Context>
concept IsVirtualContext = IsContext<Context> and requires {
    { Context::isVirtualContext(detail::IsVirtualContextTag()) } -> std::same_as<detail::Decompress<Context>>;
};

ARC_MODULE_EXPORT
struct INode;

ARC_MODULE_EXPORT
template<IsInterface... Interfaces>
struct Virtual;

}

#endif // INCLUDE_ARC_VIRTUAL_FWD_HPP
