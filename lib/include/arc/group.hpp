#ifndef INCLUDE_ARC_GROUP_HPP
#define INCLUDE_ARC_GROUP_HPP

#include "arc/context_fwd.hpp"
#include "arc/empty_types.hpp"
#include "arc/macros.hpp"
#include "arc/map_info.hpp"

#if !ARC_IMPORT_STD
#include <type_traits>
#endif

namespace arc {

ARC_MODULE_EXPORT
struct GroupBase
{
    // Base group disallows all connections
    static void enableConnectionsTo(void*) = delete;
    static void enableConnectionsFrom(void*) = delete;
};

ARC_MODULE_EXPORT
struct DefaultGroup : GroupBase
{
    // Default allows all connections to other classifications
    static void enableConnectionsTo(void*);
};

ARC_MODULE_EXPORT
template<class T>
concept IsGroup = std::is_base_of_v<GroupBase, T> and IsStateless<T>;

ARC_MODULE_EXPORT
template<class Node>
using GroupOf = ContextOf<Node>::Info::Group;

namespace detail {
    template<class Source, class Target>
    concept GroupTo = IsGroup<Source> and IsGroup<Target> and requires (Target* target) {
        Source::enableConnectionsTo(target);
    };
    template<class Target, class Source>
    concept GroupFrom = IsGroup<Source> and IsGroup<Target> and requires (Source* source) {
        Target::enableConnectionsFrom(source);
    };
}

ARC_MODULE_EXPORT
template<class SourceGroup, class TargetGroup>
concept PermissibleGroup = detail::GroupTo<SourceGroup, TargetGroup> or detail::GroupFrom<TargetGroup, SourceGroup>;

ARC_MODULE_EXPORT
template<class Source, class Target>
concept PermissibleNode = PermissibleGroup<GroupOf<Source>, GroupOf<Target>>;

namespace detail {
    template<IsGroup... Groups>
    struct JoinedGroup : Groups...
    {
        template<class Target>
        requires (detail::GroupTo<Groups, Target> or ...)
        static void enableConnectionsTo(Target*);

        template<class Source>
        requires (detail::GroupFrom<Groups, Source> or ...)
        static void enableConnectionsFrom(Source*);
    };

    template<class Group>
    auto joinGroups() -> Group;
    template<class... Groups>
    requires (sizeof...(Groups) > 1)
    auto joinGroups() -> JoinedGroup<Groups...>;

    template<IsGroup... Groups>
    struct WithGroup
    {
        template<IsContext Context>
        struct MapInfo : Context::Info
        {
            using Group = decltype(joinGroups<Groups...>());
        };
    };

} // namespace detail

ARC_MODULE_EXPORT
template<IsNodeHandle Node, IsGroup... Group>
requires (sizeof...(Group) > 0)
using WithGroup = MapInfo<Node, detail::WithGroup<Group...>>;

} // namespace arc

#endif // INCLUDE_ARC_GROUP_HPP
