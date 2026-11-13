#ifndef INCLUDE_ARC_DEPENDS_HPP
#define INCLUDE_ARC_DEPENDS_HPP

#include "arc/global_context.hpp"
#include "arc/global_trait.hpp"
#include "arc/link.hpp"
#include "arc/context_fwd.hpp"
#include "arc/macros.hpp"
#include "arc/resolve.hpp"
#include "arc/static_assert.hpp"
#include "arc/trait.hpp"
#include "arc/test_context.hpp"

#if !ARC_IMPORT_STD
#include <type_traits>
#endif

namespace arc {

// In a generated cluster's arc::Depends<>, a link for ParentTrait that the cluster's node in ClientContext takes from
// the cluster's parent, asking for it as ClientTrait (which a renaming link changes), with `[[X*]]` when Partial.
// Resolution sees ParentTrait alone; the trunk checks count what the node uses of it. ClientContext is void where
// the cluster passes the link back out to its own parent, which no node of it uses.
ARC_MODULE_EXPORT
template<class ParentTrait, class ClientTrait, class ClientContext, bool Partial>
struct ParentUse
{};

namespace detail {
    template<class Item>
    struct DependsTraitOf
    {
        using type = Item;
    };

    template<class Item>
    struct DependsTraitOf<Item*> : DependsTraitOf<Item>
    {};

    template<class ParentTrait, class ClientTrait, class ClientContext, bool Partial>
    struct DependsTraitOf<ParentUse<ParentTrait, ClientTrait, ClientContext, Partial>>
    {
        using type = ParentTrait;
    };

    // The trait an arc::Depends<> item asks for
    template<class Item>
    using DependsTrait = DependsTraitOf<Item>::type;

    template<class Item>
    concept IsDependsItem = IsTrait<DependsTrait<Item>>;

    // The item as resolution checks it: its trait, optional when the item is
    template<class Item>
    using DependsRequirement = std::conditional_t<std::is_pointer_v<Item>, DependsTrait<Item>*, DependsTrait<Item>>;

    template<class Node, class Requirement, bool Transitive>
    requires HasLocalLink<ContextOf<Node>, Requirement>
        and (not Transitive or detail::ResolveTraitFromNode<Node, Requirement>::template HasTrait<>)
    auto dependencySatisfied() -> void;

    template<class Node, IsGlobalTrait Requirement, bool Transitive>
    // If Node is within global context, we cannot check if the global node/cluster has the required trait
    requires (IsGlobalContext<ContextOf<Node>> or ContextHasGlobalTrait<ContextOf<Node>, Requirement> or (test::IsTestContext<ContextOf<Node>> and HasExplicitLink<ContextOf<Node>, Requirement>))
         and (not Transitive or detail::ResolveTraitFromNode<Node, Requirement>::template HasTrait<>)
    auto dependencySatisfied() -> void;

    // When dependency is a pointer, it is optional and not to be enforced
    template<class Node, class Requirement, bool>
    requires std::is_pointer_v<Requirement>
    auto dependencySatisfied() -> void;

    // On failure, return the requirement type for better error messages
    template<class Node, class Requirement, bool>
    auto dependencySatisfied() -> Requirement;

    struct DependsImplicitly
    {
        static constexpr bool isSpecified = false;

        // When no dependencies are specified, all dependencies are implicit and therefore allowed
        template<class, IsTrait>
        static constexpr bool dependencyListed = true;

        template<class, bool>
        using AssertSatisfied = void;
    };

    // The items listed in an arc::Depends<>, in order
    template<class... Items>
    struct DependsItems
    {};

    // Among the items of a wrapper's dependencies, everything the wrapped Node uses
    template<class Node>
    struct UsesOf
    {};

    // The dependencies of a node that wraps others, which list their own: implicit, but naming the wrapped nodes for
    // the trunk checks of a link the wrapper holds
    template<class... Nodes>
    struct DependsThrough : DependsImplicitly
    {
        using Items = DependsItems<UsesOf<Nodes>...>;
    };

    template<class Context, class Trait>
    concept ImplicitDependencyAllowed = IsTrait<Trait> and requires { Context::Info::implicitDependencyAllowed(Trait{}); };
}

ARC_MODULE_EXPORT
template<detail::IsDependsItem... Traits>
struct Depends
{
    static constexpr bool isSpecified = true;

    using Items = detail::DependsItems<Traits...>;

    // When dependencies are specified, all dependencies must be listed explicitly
    template<class Node, IsTrait Trait>
    static constexpr bool dependencyListed = detail::ImplicitDependencyAllowed<ContextOf<Node>, Trait> or (... or MatchesTrait<Trait, detail::DependsTrait<Traits>>);

    // On failure, the missing required trait types are named in a list for better error messages
    template<class Node, bool Transitive>
    using AssertSatisfied = detail::AllVoid<decltype(detail::dependencySatisfied<Node, detail::DependsRequirement<Traits>, Transitive>())...>;

    // Comma separated names of the unsatisfied dependencies, for ARC_STATIC_ASSERT_INLINE messages
    template<class Node, bool Transitive>
    static consteval std::string missingDependencies()
    {
        std::string missing;
        ([&]
        {
            if constexpr (not std::is_void_v<decltype(detail::dependencySatisfied<Node, detail::DependsRequirement<Traits>, Transitive>())>)
                missing.append(missing.empty() ? "" : ", ").append(arc::typeName<detail::DependsTrait<Traits>>);
        }(), ...);
        return missing;
    }
};

} // namespace arc

#endif // INCLUDE_ARC_DEPENDS_HPP
