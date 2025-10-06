#ifndef INCLUDE_ARC_REPEATER_HPP
#define INCLUDE_ARC_REPEATER_HPP

#include "arc/context_fwd.hpp"
#include "arc/finalise.hpp"
#include "arc/node.hpp"
#include "arc/macros.hpp"
#include "arc/resolve.hpp"
#include "arc/trait.hpp"
#include "arc/traits.hpp"

#if !ARC_IMPORT_STD
#include <cstddef>
#include <utility>
#endif

namespace arc {

ARC_MODULE_EXPORT
template<std::size_t>
struct RepeaterTrait : arc::UnconstrainedTrait
{};

ARC_MODULE_EXPORT
template<class Trait, std::size_t Count>
requires (Count > 0)
struct Repeater
{
    template<class Context>
    struct Node : arc::Node
    {
        using Traits = arc::Traits<Node, Trait>;

        template<std::size_t I>
        struct TypesAtT : arc::ResolveTypes<Node, RepeaterTrait<I>>
        {
            static constexpr std::size_t TypesCount = Count;
            template<std::size_t Index>
            using TypesAt = TypesAtT<Index>;
        };

        using Types = TypesAtT<0>;

        template<class Source, class Key = ContextOf<Source>::Info::DefaultKey>
        ARC_INLINE constexpr auto finalise(this auto& self, Source& source, Key const& key = {}, auto const&... keys)
        {
            // Don't consume the key, as it needs to be applied for each repeater trait
            return arc::finalise<false>(source, self, key, keys...);
        }

        ARC_INLINE constexpr void implWithKey(this auto& self, auto const& key, auto const& keys, auto&&... args)
        {
            self.applyWithKey(std::make_index_sequence<Count>{}, key, keys, args...);
        }

    private:
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

} // namespace arc


#endif // INCLUDE_ARC_REPEATER_HPP
