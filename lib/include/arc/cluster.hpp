#ifndef INCLUDE_ARC_CLUSTER_HPP
#define INCLUDE_ARC_CLUSTER_HPP

#include "arc/detail/as_ref.hpp"
#include "arc/context_fwd.hpp"
#include "arc/depends.hpp"
#include "arc/environment.hpp"
#include "arc/link.hpp"
#include "arc/macros.hpp"
#include "arc/node_fwd.hpp"
#include "arc/trait.hpp"
#include "arc/trait_view.hpp"

#if !ARC_IMPORT_STD
#include <compare>
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
struct Cluster
{
    static constexpr bool isUnary() { return false; }
    using Environment = arc::Environment<>;
    using Depends = detail::DependsImplicitly;

    constexpr auto* operator->(this auto& self)
    {
        return std::addressof(self);
    }

    template<class Self>
    requires IsRootContext<ContextParameterOf<Self>>
    ARC_INLINE void onConstructed(this Self& self)
    {
        self.visit(detail::OnGraphConstructedVisitor{});
    }

    template<class Self, IsTrait Trait, class Visitor>
    requires IsRootContext<ContextParameterOf<Self>>
    void visitTrait(this Self& self, Trait, Visitor&& visitor)
    {
        self.visit(detail::TraitVisitor<Trait, std::remove_cvref_t<Visitor>>{visitor});
    }

    template<IsTrait Trait, class Self, class Key = ContextParameterOf<Self>::Info::DefaultKey>
    constexpr IsTraitViewOf<Trait, Key> auto getNode(this Self& cluster, Trait trait = {}, Key key = {}, auto const&... keys)
    {
        auto target = cluster.getNode(detail::AsRef{}, trait);
        return makeTraitView(cluster, target, trait, key, keys...);
    }

    template<IsTrait Trait, class Self, class Key = ContextParameterOf<Self>::Info::DefaultKey>
    constexpr IsTraitViewOf<Trait, Key> auto getGlobal(this Self& cluster, Trait trait = {}, Key key = {}, auto const&... keys)
    {
        return cluster.getNode(arc::global(trait), key, keys...);
    }

    template<IsTrait Trait, class Self>
    constexpr auto getNode(this Self& cluster, detail::AsRef, Trait trait = {})
    {
        return detail::getContextParameter(cluster).getNode(cluster, trait);
    }

    template<IsTrait Trait, class Self>
    constexpr auto canGetNode(this Self const&, Trait = {})
    {
        constexpr bool value =
            requires (Self c, Trait trait) {
                detail::getContextParameter(c).getNode(c, trait);
            };
        return std::bool_constant<value>{};
    }

    template<IsTrait Trait, class Self, class Key = ContextParameterOf<Self>::Info::DefaultKey>
    requires detail::HasLink<Self, Trait>
    constexpr IsTraitViewOf<Trait, Key> auto asTrait(this Self& self, Trait trait = {}, Key key = {}, auto const&... keys)
    {
        auto target = self.asTrait(detail::AsRef{}, trait);
        return makeTraitView(self, target, trait, key, keys...);
    }

    template<class Self, IsTrait Trait>
    requires detail::HasLink<Self, Trait>
    constexpr auto asTrait(this Self& cluster, detail::AsRef asRef, Trait)
    {
        Self::template ensureDepth<ContextParameterOf<Self>>();
        using Target = detail::ResolveLink<Self, Trait>;
        auto memPtr = getNodePointer(AdlTag<typename Target::Context>{});
        auto& node = memPtr.getMemberFromClass(cluster);
        return node.asTrait(asRef, typename Target::Trait{});
    }

    template<IsTrait Trait, class Self>
    constexpr auto hasTrait(this Self const&, Trait = {})
    {
        constexpr bool value =
            detail::HasLink<Self, Trait> and
            requires (AdlTag<detail::ResolveLinkContext<Self, Trait>> adlTag) {
                getNodePointer(adlTag);
            };
        return std::bool_constant<value>{};
    }

    template<class Context>
    static consteval void ensureDepth() {}
};

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

#if !ARC_COMPILER_MSVC
    auto operator<=>(DomainParams const&) const = default;
#endif
};

ARC_MODULE_EXPORT
template<DomainParams Params = {}>
struct Domain : Cluster
{
    template<class Context>
    static consteval void ensureDepth()
    {
        static_assert(Context::Depth <= Params.MaxDepth);
    }
};

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


#endif // INCLUDE_ARC_CLUSTER_HPP
