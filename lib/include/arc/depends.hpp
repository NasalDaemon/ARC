#ifndef INCLUDE_ARC_REQUIRES_HPP
#define INCLUDE_ARC_REQUIRES_HPP

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
    template<class Context, class Requirement, bool Transitive>
    requires HasLink<Context, Requirement>
        and (not Transitive or detail::ResolveTrait<Context, Requirement>::template HasTrait<>)
    auto dependencySatisfied() -> void;

    template<class Context, IsGlobalTrait Requirement, bool Transitive>
    requires ContextHasGlobalTrait<Context, Requirement>
        and (not Transitive or detail::ResolveTrait<Context, Requirement>::template HasTrait<>)
    auto dependencySatisfied() -> void;

    // When dependency is a pointer, it is optional and not to be enforced
    template<class Context, class Requirement, bool>
    requires std::is_pointer_v<Requirement>
    auto dependencySatisfied() -> void;

    // On failure, return the requirement type for better error messages
    template<class Context, class Requirement, bool>
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
}

ARC_MODULE_EXPORT
template<class... Traits>
requires (... and IsTrait<std::remove_pointer_t<Traits>>)
struct Depends
{
    static constexpr bool isSpecified = true;

    // When dependencies are specified, all dependencies must be listed explicitly
    template<class Node, IsTrait Trait>
    static constexpr bool dependencyListed = requires { ContextOf<Node>::Info::implicitDependencyAllowed(Trait{}); } or (... or MatchesTrait<Trait, std::remove_pointer_t<Traits>>);

    // On failure, the missing required trait types are named in a list for better error messages
    template<class Node, bool Transitive>
    using AssertSatisfied = detail::AllVoid<decltype(detail::dependencySatisfied<ContextOf<Node>, Traits, Transitive>())...>;
};

} // namespace arc

#endif // INCLUDE_ARC_REQUIRES_HPP
