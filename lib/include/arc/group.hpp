#ifndef INCLUDE_ARC_GROUP_HPP
#define INCLUDE_ARC_GROUP_HPP

#include "arc/context_fwd.hpp"
#include "arc/empty_types.hpp"
#include "arc/macros.hpp"
#include "arc/map_info.hpp"

#if !ARC_IMPORT_STD
#include <concepts>
#include <type_traits>
#endif

namespace arc {

ARC_MODULE_EXPORT
struct GroupWriteAccess{};

ARC_MODULE_EXPORT
struct Group;

ARC_MODULE_EXPORT
template<class T>
concept IsGroup = std::is_base_of_v<Group, T> and IsStateless<T>;

namespace detail {
    struct JoinedGroupBase;
}

ARC_MODULE_EXPORT
template<class T>
concept IsSingleGroup = IsGroup<T> and not std::derived_from<T, detail::JoinedGroupBase>;

namespace detail {
    template<IsSingleGroup... Groups>
    requires (sizeof...(Groups) > 1)
    struct JoinedGroup;
}

ARC_MODULE_EXPORT
template<class Node>
using GroupOf = ContextOf<Node>::Info::Group;

namespace detail {
    template<class Source, class Target>
    concept IsGroupReadTo = IsGroup<Source> and IsGroup<Target> and requires (Source source, Target const* target) {
        source.connectionsTo(target);
    };
    template<class Source, class Target>
    concept IsGroupWriteTo = IsGroupReadTo<Source, Target> and requires (Source source, Target const* target) {
        { source.connectionsTo(target) } -> std::same_as<GroupWriteAccess>;
    };
    template<class Target, class Source>
    concept IsGroupReadFrom = IsGroup<Source> and IsGroup<Target> and requires (Target target, Source const* source) {
        target.connectionsFrom(source);
    };
    template<class Target, class Source>
    concept IsGroupWriteFrom = IsGroupReadFrom<Target, Source> and requires (Target target, Source const* source) {
        { target.connectionsFrom(source) } -> std::same_as<GroupWriteAccess>;
    };
}

ARC_MODULE_EXPORT
struct Group
{
    template<class Source>
    auto connectionsTo(this Source, Source const*) -> GroupWriteAccess;

    // Write access only if all target groups allow write from the source
    template<class Source, class... Targets>
    requires (detail::IsGroupReadTo<Source, Targets> or ...)
    auto connectionsTo(this Source, detail::JoinedGroup<Targets...> const*)
        -> std::conditional_t<(detail::IsGroupWriteTo<Source, Targets> and ...), GroupWriteAccess, void>;

    template<class Target>
    auto connectionsFrom(this Target, Target const*) -> GroupWriteAccess;

    // Write access if the target group allows write from one of the sources
    template<class Target, class... Sources>
    requires (detail::IsGroupReadFrom<Target, Sources> or ...)
    auto connectionsFrom(this Target, detail::JoinedGroup<Sources...> const*)
        -> std::conditional_t<(detail::IsGroupWriteFrom<Target, Sources> or ...), GroupWriteAccess, void>;
};

// Default group for unclassified nodes
ARC_MODULE_EXPORT
struct NoGroup : Group
{
    // Default group can only connect to itself (read+write access)
    static GroupWriteAccess connectionsTo(NoGroup const*);
    static GroupWriteAccess connectionsFrom(NoGroup const*);
};

ARC_MODULE_EXPORT
template<class SourceGroup, class TargetGroup>
concept IsReadPermittedGroup = detail::IsGroupReadTo<SourceGroup, TargetGroup> or detail::IsGroupReadFrom<TargetGroup, SourceGroup>;

ARC_MODULE_EXPORT
template<class SourceGroup, class TargetGroup>
concept IsWritePermittedGroup = IsReadPermittedGroup<SourceGroup, TargetGroup>
                            and (detail::IsGroupWriteTo<SourceGroup, TargetGroup> or detail::IsGroupWriteFrom<TargetGroup, SourceGroup>);

ARC_MODULE_EXPORT
template<class SourceGroup, class TargetGroup>
concept IsReadOnlyPermittedGroup = IsReadPermittedGroup<SourceGroup, TargetGroup> and not IsWritePermittedGroup<SourceGroup, TargetGroup>;

ARC_MODULE_EXPORT
template<class Source, class Target>
concept IsReadPermittedNode = IsReadPermittedGroup<GroupOf<Source>, GroupOf<Target>>;

ARC_MODULE_EXPORT
template<class Source, class Target>
concept IsWritePermittedNode = IsReadPermittedNode<Source, Target>
                           and IsWritePermittedGroup<GroupOf<Source>, GroupOf<Target>>;

ARC_MODULE_EXPORT
template<class Source, class Target>
concept IsReadOnlyPermittedNode = IsReadPermittedNode<Source, Target> and not IsWritePermittedNode<Source, Target>;

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

} // namespace arc

#endif // INCLUDE_ARC_GROUP_HPP
