#ifndef INCLUDE_ARC_GROUP_HPP
#define INCLUDE_ARC_GROUP_HPP

#include "arc/context_fwd.hpp"
#include "arc/empty_types.hpp"
#include "arc/macros.hpp"
#include "arc/nodes/map_info.hpp"

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
    // Default group allows connections to/from any group as long as the counterpart allows it
    static GroupWriteAccess connectionsTo(Group const*);
    static GroupWriteAccess connectionsFrom(Group const*);
};

ARC_MODULE_EXPORT
template<class SourceGroup, class TargetGroup>
concept IsReadPermittedGroup = detail::IsGroupReadTo<SourceGroup, TargetGroup> and detail::IsGroupReadFrom<TargetGroup, SourceGroup>;

ARC_MODULE_EXPORT
template<class SourceGroup, class TargetGroup>
concept IsWritePermittedGroup = IsReadPermittedGroup<SourceGroup, TargetGroup>
                            and (detail::IsGroupWriteTo<SourceGroup, TargetGroup> and detail::IsGroupWriteFrom<TargetGroup, SourceGroup>);

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

} // namespace arc

#endif // INCLUDE_ARC_GROUP_HPP
