#ifndef INCLUDE_ARC_NODE_MAP_INFO_HPP
#define INCLUDE_ARC_NODE_MAP_INFO_HPP

#include "arc/detail/concepts.hpp"
#include "arc/detail/compress.hpp"

#include "arc/context_fwd.hpp"
#include "arc/empty_types.hpp"
#include "arc/trait.hpp"
#include "arc/macros.hpp"
#include "arc/static_assert.hpp"
#include "arc/node_fwd.hpp"

namespace arc {

ARC_MODULE_EXPORT
template<class T>
concept IsInfoMapper = requires {
    typename detail::TakesUnaryClassTemplate<T::template MapInfo>;
} and IsStateless<T>;

ARC_MODULE_EXPORT
template<IsNodeHandle NodeHandle, IsInfoMapper InfoMapper>
struct MapInfo
{
    template<class Context>
    struct MappedContext : Context
    {
        using NodePath = detail::NodePathOf<Context>;

        template<class NH>
        friend auto innerNodeHandle(MappedContext*, AdlTag<NH>) -> arc::InnerNodeHandle<Context, NodeHandle>;

        friend consteval auto getNodePointer(AdlTag<MappedContext>)
        {
            return getNodePointer(AdlTag<Context>{});
        }

        using Info = InfoMapper::template MapInfo<detail::CompressContext<Context>>;

        ARC_STATIC_ASSERT(std::derived_from<Info, typename Context::Info>)("Mapped Info must derive from the original context's Info",
            (mapper, arc::typeName<InfoMapper>)(info, arc::typeName<Info>)(original_info, arc::typeName<typename Context::Info>));
    };

    template<class Context>
    using Node = ToNodeWrapper<NodeHandle>::template Node<
        detail::CompressContext<MappedContext<detail::Decompress<Context>>>>;
};

namespace node {
    ARC_MODULE_EXPORT
    using arc::MapInfo;
}

}

#endif // INCLUDE_ARC_NODE_MAP_INFO_HPP
