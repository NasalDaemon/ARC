#ifndef INCLUDE_ARC_DEPENDS_HPP
#define INCLUDE_ARC_DEPENDS_HPP

#include "arc/global_context.hpp"
#include "arc/global_trait.hpp"
#include "arc/link.hpp"
#include "arc/context_fwd.hpp"
#include "arc/macros.hpp"
#include "arc/resolve.hpp"
#include "arc/trait.hpp"

#if !ARC_IMPORT_STD
#include <type_traits>
#endif

namespace arc {

namespace detail {
    template<class Trait>
    concept IsDependsItem = IsTrait<std::remove_pointer_t<Trait>>;

    template<IsDependsItem Item>
    using DependsTrait = std::remove_pointer_t<Item>;

    template<class Node, class Requirement, bool Transitive>
    requires HasLink<ContextOf<Node>, Requirement>
        and (not Transitive or detail::ResolveTraitFromNode<Node, Requirement>::template HasTrait<>)
    auto dependencySatisfied() -> void;

    template<class Node, IsGlobalTrait Requirement, bool Transitive>
    requires ContextHasGlobalTrait<ContextOf<Node>, Requirement>
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

    template<class Context, class Trait>
    concept ImplicitDependencyAllowed = IsTrait<Trait> and requires { Context::Info::implicitDependencyAllowed(Trait{}); };
}

ARC_MODULE_EXPORT
template<detail::IsDependsItem... Traits>
struct Depends
{
    static constexpr bool isSpecified = true;

    // When dependencies are specified, all dependencies must be listed explicitly
    template<class Node, IsTrait Trait>
    static constexpr bool dependencyListed = detail::ImplicitDependencyAllowed<ContextOf<Node>, Trait> or (... or MatchesTrait<Trait, detail::DependsTrait<Traits>>);

    // On failure, the missing required trait types are named in a list for better error messages
    template<class Node, bool Transitive>
    using AssertSatisfied = detail::AllVoid<decltype(detail::dependencySatisfied<Node, Traits, Transitive>())...>;
};

} // namespace arc

#endif // INCLUDE_ARC_DEPENDS_HPP
