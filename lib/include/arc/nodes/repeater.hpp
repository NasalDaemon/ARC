#ifndef INCLUDE_ARC_REPEATER_HPP
#define INCLUDE_ARC_REPEATER_HPP

#include "arc/context_fwd.hpp"
#include "arc/detail/with_index.hpp"
#include "arc/finalise.hpp"
#include "arc/key.hpp"
#include "arc/node.hpp"
#include "arc/macros.hpp"
#include "arc/node_fwd.hpp"
#include "arc/resolve.hpp"
#include "arc/trait.hpp"
#include "arc/traits.hpp"

#if !ARC_IMPORT_STD
#include <cstddef>
#include <type_traits>
#include <utility>
#endif

namespace arc {

ARC_MODULE_EXPORT
template<std::size_t>
struct RepeaterTrait : arc::UnconstrainedTrait
{};

namespace key {
    ARC_MODULE_EXPORT
    template<std::size_t I>
    struct RepeaterIndex : Default
    {};
    ARC_MODULE_EXPORT
    template<std::size_t I>
    inline constexpr RepeaterIndex<I> repeaterIndex{};

    ARC_MODULE_EXPORT
    template<IsNodeHandle N>
    struct RepeaterNode : Default
    {};
    ARC_MODULE_EXPORT
    template<IsNodeHandle N>
    inline constexpr RepeaterNode<N> repeaterNode{};
}

ARC_MODULE_EXPORT
template<class Trait, std::size_t Count>
requires (Count > 0)
struct Repeater
{
    template<class Context>
    struct Node : arc::Node
    {
        using Traits = arc::Traits<Trait>;

        template<std::size_t I>
        struct TypesAtT : arc::ResolveTypes<Node, RepeaterTrait<I>>
        {
            static constexpr std::size_t TypesCount = Count;
            template<std::size_t Index>
            using TypesAt = TypesAtT<Index>;
        };

        using Types = TypesAtT<0>;

        template<class Source, std::size_t I>
        ARC_INLINE constexpr auto finalise(this auto& self, Source& source, key::RepeaterIndex<I>, auto const&... keys)
        {
            auto target = Context{}.getNode(detail::upCast<Node>(self), RepeaterTrait<I>{});
            return target.ptr->finalise(source, keys...);
        }

        template<class Source, IsNodeHandle N>
        ARC_INLINE constexpr auto finalise(this auto& self, Source& source, key::RepeaterNode<N>, auto const&... keys)
        {
            static constexpr auto index = indexOf<N>(std::make_index_sequence<Count>{});
            return self.finalise(source, key::repeaterIndex<index>, keys...);
        }
        template<class Source, class Key = ContextOf<Source>::Info::DefaultKey>
        ARC_INLINE constexpr auto finalise(this auto& self, Source& source, Key const& key = {}, auto const&... keys)
        {
            // Don't consume the key, as it needs to be applied for each repeater trait
            return arc::finalise<false>(source, self, key, keys...);
        }

        static auto finaliseTypes(auto const&...) -> Types;
        template<std::size_t I>
        static auto finaliseTypes(key::RepeaterIndex<I>, auto const&...) -> arc::ResolveTypes<Node, RepeaterTrait<I>>;
        template<IsNodeHandle N>
        static auto finaliseTypes(key::RepeaterNode<N>, auto const&...) -> arc::ResolveTypes<Node, RepeaterTrait<indexOf<N>(std::make_index_sequence<Count>{})>>;

        template<class Source, class Self, class Types, class... Keys>
        using FinaliseTypes = decltype(finaliseTypes(std::declval<Keys const&>()...));

        ARC_INLINE constexpr void implWithKey(this auto& self, auto const& key, auto const& keys, auto&&... args)
        {
            self.applyWithKey(std::make_index_sequence<Count>{}, key, keys, args...);
        }

    private:
        template<IsNodeHandle N, std::size_t... Is>
        static consteval std::size_t indexOf(std::index_sequence<Is...>)
        {
            return arc::indexOf<N, typename ContextOf<typename detail::ResolveTraitFromNode<Node, RepeaterTrait<Is>>::Node>::NodeHandle...>();
        }

        template<std::size_t... Is>
        constexpr void applyWithKey(this auto& self, std::index_sequence<Is...>, auto const& key, auto const& keys, auto&... args)
        {
            (self.applyWithKey2(Context{}.getNode(detail::upCast<Node>(self), RepeaterTrait<Is>{}), key, keys, args...), ...);
        }

        constexpr void applyWithKey2(this auto& self, auto target, auto const& key, auto const& keys, auto&... args)
        {
            std::apply(
                [&](auto const&... ks)
                {
                    target.ptr->finalise(self, key, ks...)->impl(args...);
                },
                keys);
        }
    };
};

namespace node {
    ARC_MODULE_EXPORT
    using arc::Repeater;
}

} // namespace arc


#endif // INCLUDE_ARC_REPEATER_HPP
