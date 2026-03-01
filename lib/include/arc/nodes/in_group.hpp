#ifndef INCLUDE_ARC_NODES_IN_GROUP_HPP
#define INCLUDE_ARC_NODES_IN_GROUP_HPP

#include "arc/group.hpp"
#include "arc/nodes/map_info.hpp"

#if !ARC_IMPORT_STD
#include <type_traits>
#endif

namespace arc {

namespace detail {
    // Check if any group in Groups can connect to Target
    template<class Target, class... Groups>
    concept AnyGroupReadTo = (IsGroupReadTo<Groups, Target> or ...);

    template<class Target, class... Groups>
    concept AnyGroupWriteTo = (IsGroupWriteTo<Groups, Target> or ...);

    template<class Source, class... Groups>
    concept AnyGroupReadFrom = (IsGroupReadFrom<Groups, Source> or ...);

    template<class Source, class... Groups>
    concept AllGroupsWriteFrom = (IsGroupWriteFrom<Groups, Source> and ...);

    struct JoinedGroupBase : Group {};

    template<IsSingleGroup... Groups>
    requires (sizeof...(Groups) > 1)
    struct JoinedGroup : JoinedGroupBase
    {
        // Write access only if at least one source group can write to the target
        template<AnyGroupReadTo<Groups...> Target>
        static auto connectionsTo(Target const*) -> std::conditional_t<AnyGroupWriteTo<Target, Groups...>, GroupWriteAccess, void>;

        // Write access only if all target groups can be written to by at least one source
        template<IsSingleGroup... TargetGroups>
        requires (AnyGroupReadTo<TargetGroups, Groups...> or ...)
        static auto connectionsTo(JoinedGroup<TargetGroups...> const*)
            -> std::conditional_t<(AnyGroupWriteTo<TargetGroups, Groups...> and ...), GroupWriteAccess, void>;

        // Write access only if all target groups can be written to by the source
        template<AnyGroupReadFrom<Groups...> Source>
        static auto connectionsFrom(Source const*) -> std::conditional_t<AllGroupsWriteFrom<Source, Groups...>, GroupWriteAccess, void>;

        // Write access only if all target groups can be written to by at least one source
        template<IsSingleGroup... SourceGroups>
        requires (AnyGroupReadFrom<SourceGroups, Groups...> or ...)
        static auto connectionsFrom(JoinedGroup<SourceGroups...> const*)
            -> std::conditional_t<(AllGroupsWriteFrom<SourceGroups, Groups...> or ...), GroupWriteAccess, void>;
    };

    template<IsSingleGroup Group>
    auto joinGroups() -> Group;
    template<IsSingleGroup... Groups>
    requires (sizeof...(Groups) > 1)
    auto joinGroups() -> JoinedGroup<Groups...>;

    template<IsSingleGroup... Groups>
    struct InGroup
    {
        template<IsContext Context>
        struct MapInfo : Context::Info
        {
            using Group = decltype(joinGroups<Groups...>());
        };
    };

} // namespace detail

ARC_MODULE_EXPORT
template<IsNodeHandle Node, IsSingleGroup... Groups>
requires (sizeof...(Groups) > 0)
struct InGroup : MapInfo<Node, detail::InGroup<Groups...>> {};

namespace node {
    ARC_MODULE_EXPORT
    using arc::InGroup;
}

} // namespace arc

#endif // INCLUDE_ARC_NODES_IN_GROUP_HPP
