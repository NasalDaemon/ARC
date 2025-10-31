#ifndef INCLUDE_ARC_DETAIL_AS_REF_HPP
#define INCLUDE_ARC_DETAIL_AS_REF_HPP

#include "arc/detail/compress.hpp"
#include "arc/macros.hpp"
#include "arc/node_fwd.hpp"
#include "arc/empty_types.hpp"
#include "arc/virtual_fwd.hpp"

#if !ARC_IMPORT_STD
#include <memory>
#include <type_traits>
#endif

namespace arc {

ARC_MODULE_EXPORT
struct Node;
ARC_MODULE_EXPORT
struct Cluster;
ARC_MODULE_EXPORT
template<IsNodeHandle NodeHandle, class ID>
struct Collection;

namespace detail {
    struct ContextBase;

    // Bypass TraitView instantiation during intermediate asTrait/getNode calls
    struct AsRef
    {
    private:
        friend struct arc::Node;
        friend struct arc::Cluster;
        friend struct arc::detail::ContextBase;
        template<IsNodeHandle NodeHandle, class ID>
        friend struct arc::Collection;
        template<IsInterface... Interfaces>
        friend struct arc::Virtual;

        AsRef() = default;
    };

    template<class Interface_, class Types = EmptyTypes>
    struct TargetRef
    {
        using Interface = Interface_;
        explicit constexpr TargetRef(Interface& ref, std::type_identity<Types>) : ptr(std::addressof(ref)) {}
        Interface* ptr;
        static constexpr std::type_identity<CompressTypes<Types>> types() { return {}; };
        static constexpr std::type_identity<EmptyTypes> types() requires std::is_same_v<Types, EmptyTypes> { return {}; };
    };

    template<class Interface, class Types>
    TargetRef(Interface&, std::type_identity<Types>) -> TargetRef<Interface, Types>;

} // namespace detail

} // namespace arc

#endif // INCLUDE_ARC_DETAIL_AS_REF_HPP
