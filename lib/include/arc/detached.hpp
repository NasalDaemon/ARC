#ifndef INCLUDE_ARC_DETACHED_HPP
#define INCLUDE_ARC_DETACHED_HPP

#include "arc/detail/cast.hpp"
#include "arc/empty_types.hpp"
#include "arc/macros.hpp"
#include "arc/node_fwd.hpp"
#include "arc/context_fwd.hpp"

#if !ARC_IMPORT_STD
#include <memory>
#include <type_traits>
#endif

namespace arc {

struct Node;

ARC_MODULE_EXPORT
struct DetachedInterface {};

ARC_MODULE_EXPORT
template<class T>
concept IsDetachedInterface = std::is_base_of_v<DetachedInterface, T> and not std::is_base_of_v<Node, T>;
ARC_MODULE_EXPORT
template<class T>
concept IsDetachedImpl = std::is_base_of_v<DetachedInterface, T> and std::is_base_of_v<Node, T>;

// When context is detached from the trait, then getNode and asNode calls are only available when deducing this.
// This means that the overlaid environment is also available when getNode is called.
ARC_MODULE_EXPORT
template<class T>
concept HasDetachedContext = IsWrappedImpl<T> or IsDetachedImpl<T>;

ARC_MODULE_EXPORT
template<class Node, IsDetachedInterface Interface>
struct DetachedImpl : Interface, private Node
{
    static_assert(IsStateless<Interface>);

    ARC_NODE_USE_PUBLIC_MEMBERS(Node)
    using Types = Node::Types;
    using Node::finalise;

    // Use impl from the detached interface
    using Interface::impl;

    template<class Self>
    constexpr decltype(auto) visit(this Self& self, auto&&... args)
    {
        ContextOf<Self>::Info::assertVisitable(self);
        return self.Node::visit(ARC_FWD(args)...);
    }

    template<class Self>
    constexpr auto& getState(this Self& self)
    {
        ContextOf<Self>::Info::assertAccessible(self);
        return detail::upCast<Node>(self);
    }

    constexpr auto* operator->(this auto& self) { return std::addressof(self.getState()); }
};

}


#endif // INCLUDE_ARC_DETACHED_HPP
