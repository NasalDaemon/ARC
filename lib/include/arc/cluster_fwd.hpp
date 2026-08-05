#ifndef INCLUDE_ARC_CLUSTER_FWD_HPP
#define INCLUDE_ARC_CLUSTER_FWD_HPP

#include "arc/detail/as_ref.hpp"
#include "arc/context_fwd.hpp"
#include "arc/macros.hpp"

#if !ARC_IMPORT_STD
#include <concepts>
#include <type_traits>
#endif

namespace arc {

namespace detail {
    struct OnGraphConstructedVisitor
    {
        constexpr void operator()(IsNode auto& node) const
        {
            if constexpr (requires { node.onGraphConstructed(); })
                node.onGraphConstructed();
        }
    };
    template<IsTrait Trait, class Visitor>
    struct TraitVisitor
    {
        template<IsNode Node>
        constexpr void operator()(Node& node) const
        {
            if constexpr (HasTrait<Node, Trait>)
                node.asTrait(Trait{})->visit(visitor);
        }

        Visitor const& visitor;
    };
}

ARC_MODULE_EXPORT
struct Cluster;

ARC_MODULE_EXPORT
template<class T>
concept IsCluster = std::derived_from<T, Cluster>;

ARC_MODULE_EXPORT
template<class T>
concept IsRootCluster = IsCluster<T> and IsRootContext<ContextParameterOf<T>>;

ARC_MODULE_EXPORT
struct DomainParams
{
    std::size_t MaxDepth = 3;

    auto operator<=>(DomainParams const&) const = default;
};

ARC_MODULE_EXPORT
template<DomainParams Params = {}>
struct Domain;

namespace detail {
    template<class T>
    inline constexpr bool isDomain = false;
    template<DomainParams Params>
    constexpr bool isDomain<Domain<Params>> = true;

    template<class T>
    concept IsDomain = isDomain<std::remove_cvref_t<T>>;
}

ARC_MODULE_EXPORT
template<class T>
concept IsDomain = IsCluster<T> and detail::IsDomain<T>;

ARC_MODULE_EXPORT
template<class T>
concept IsRootDomain = IsDomain<T> and IsRootCluster<T>;

} // namespace arc


#endif // INCLUDE_ARC_CLUSTER_FWD_HPP
