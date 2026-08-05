#ifndef INCLUDE_ARC_LINK_HPP
#define INCLUDE_ARC_LINK_HPP

#include "arc/context_fwd.hpp"
#include "arc/global_trait.hpp"
#include "arc/macros.hpp"
#include "arc/trait.hpp"

#if !ARC_IMPORT_STD
#include <concepts>
#endif

namespace arc {

ARC_MODULE_EXPORT
template<class T, class Trait>
concept CanGetNode = requires (T t, Trait trait) {
    { t.canGetNode(trait) } -> std::same_as<std::true_type>;
};

ARC_MODULE_EXPORT
template<class T, class Trait>
concept HasTrait = requires (T t, Trait trait) {
    { t.hasTrait(trait) } -> std::same_as<std::true_type>;
};

ARC_MODULE_EXPORT
template<class Context_, class Trait_>
struct ResolvedLink
{
    using Context = Context_;
    using Trait = Trait_;
};

ARC_MODULE_EXPORT
template<class T>
concept IsResolvedLink = IsContext<typename T::Context> and IsTrait<typename T::Trait>;

ARC_MODULE_EXPORT
struct LinkPriorityMin
{};
ARC_MODULE_EXPORT
struct LinkPriorityMax : LinkPriorityMin
{};
ARC_MODULE_EXPORT
template<class T>
struct LinkExact : LinkPriorityMax
{};

namespace detail {

    template<class T, class Trait>
    concept HasExplicitLink = IsTrait<Trait> and requires (Trait trait, LinkExact<Trait> linkExact) { { T::resolveLink(trait, linkExact) } -> IsResolvedLink; };

    template<class T, class Trait>
    concept HasImplicitLink = IsGlobalTrait<Trait> and requires { T::resolveLinkGlobal(); };

    template<class T, class Trait>
    concept HasLink = HasExplicitLink<T, Trait> or HasImplicitLink<T, Trait>;

    template<class T, class Trait>
    concept HasGlobalLink = HasLink<T, Trait> and IsGlobalTrait<Trait>;

    template<class T, class Trait>
    concept HasLocalLink = HasLink<T, Trait> and IsNonGlobalTrait<Trait>;

    template<class T, class Trait>
    requires HasExplicitLink<T, Trait>
    using ResolveLink = decltype(T::resolveLink(std::declval<Trait>(), std::declval<LinkExact<Trait>>()));

    template<class T, class Trait>
    requires HasExplicitLink<T, Trait>
    using ResolveLinkContext = ResolveLink<T, Trait>::Context;

    template<class T, class Trait>
    requires HasExplicitLink<T, Trait>
    using ResolveLinkTrait = ResolveLink<T, Trait>::Trait;

    template<class T, class Trait>
    concept LinksToParent = HasLocalLink<T, Trait> and std::same_as<typename T::ParentContext, ResolveLinkContext<T, Trait>>;

} // detail

}


#endif // INCLUDE_ARC_LINK_HPP
