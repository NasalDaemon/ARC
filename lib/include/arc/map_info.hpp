#ifndef INCLUDE_ARC_NODE_MAP_INFO_HPP
#define INCLUDE_ARC_NODE_MAP_INFO_HPP

#include "arc/detail/concepts.hpp"
#include "arc/detail/compress.hpp"

#include "arc/context_fwd.hpp"
#include "arc/empty_types.hpp"
#include "arc/global_trait.hpp"
#include "arc/link.hpp"
#include "arc/macros.hpp"
#include "arc/node_fwd.hpp"

#if !ARC_IMPORT_STD
#include <type_traits>
#endif

namespace arc {

ARC_MODULE_EXPORT
template<class T>
concept IsInfoMapper = requires {
    typename detail::TakesUnaryClassTemplate<T::template MapInfo>;
} and IsStateless<T>;

namespace detail {
    template<class Context, IsInfoMapper InfoMapper>
    struct MappedContext : Context
    {
        template<class Trait>
        requires detail::HasLink<Context, Trait> or IsGlobalTrait<Trait>
        constexpr auto getNode(this Context context, auto& node, Trait trait)
        {
            return context.getNode(node, trait);
        }

        using Info = InfoMapper::template MapInfo<Context>;

        static_assert(std::derived_from<Info, typename Context::Info>);
    };
}

ARC_MODULE_EXPORT
template<IsNodeHandle NodeHandle, IsInfoMapper InfoMapper>
struct MapInfo
{
    template<class Context>
    using Node = ToNodeWrapper<NodeHandle>::template Node<
        detail::CompressContext<detail::MappedContext<detail::Decompress<Context>, InfoMapper>>>;
};

}

#endif // INCLUDE_ARC_NODE_MAP_INFO_HPP
