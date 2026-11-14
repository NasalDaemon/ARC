#ifndef INCLUDE_ARC_TRUNK_LINKS_HPP
#define INCLUDE_ARC_TRUNK_LINKS_HPP

#include "arc/detail/type_at.hpp"
#include "arc/context_fwd.hpp"
#include "arc/depends.hpp"
#include "arc/global_trait.hpp"
#include "arc/macros.hpp"
#include "arc/node_fwd.hpp"
#include "arc/static_assert.hpp"
#include "arc/trait.hpp"
#include "arc/type_name.hpp"

#if !ARC_IMPORT_STD
#include <concepts>
#include <string>
#include <string_view>
#include <type_traits>
#endif

namespace arc {

// The links a node holds, as the generator lists them for the node's trunk checks. Each generated link derives from
// one of these and declares `static constexpr char const Pos[]`, the link's position in the DSL.

// `[[cluster::X]]` or `[[cluster::X::Name]]`: a trunk the cluster declares, which the node must use in full
ARC_MODULE_EXPORT
template<class Face>
struct TrunkLink
{
    using Trait = TrunkOf<Face>;
    using TrunkFace = Face;
    static constexpr bool isTrunk = true;
    static constexpr bool fullUse = true;
};

// `[[cluster::X*]]`: a trunk the cluster declares, of which the node may use a subset
ARC_MODULE_EXPORT
template<class Face>
struct PartialTrunkLink
{
    using Trait = TrunkOf<Face>;
    using TrunkFace = Face;
    static constexpr bool isTrunk = true;
    static constexpr bool fullUse = false;
};

// `[[A + B]]`: a trunk listed at the link
ARC_MODULE_EXPORT
template<class TrunkT>
requires IsTrunk<TrunkT>
struct InlineTrunkLink
{
    using Trait = TrunkT;
    static constexpr bool isTrunk = true;
    static constexpr bool fullUse = false;
};

// `[A]`: a link for one trait
ARC_MODULE_EXPORT
template<class Trait_>
struct TraitLink
{
    using Trait = Trait_;
    static constexpr bool isTrunk = false;
    static constexpr bool fullUse = false;
};

namespace detail {

    template<class... Traits>
    struct TrunkTraitList
    {};

    // The traits a link stands for: each of a trunk's, or the link's one trait
    template<class Trait>
    struct LinkTraitsOf
    {
        using type = TrunkTraitList<Trait>;
    };

    template<class... Traits>
    struct LinkTraitsOf<Trunk<Traits...>>
    {
        using type = TrunkTraitList<Traits...>;
    };

    template<class Link>
    using LinkTraits = LinkTraitsOf<typename Link::Trait>::type;

    // Whether a request for one of the traits could resolve through a link for the other
    template<class A, class B>
    concept TraitsOverlap = std::same_as<A, B> or MatchesTrait<A, B> or MatchesTrait<B, A>;

    template<class... Traits>
    consteval std::string joinTraitNames(TrunkTraitList<Traits...>, std::string_view separator)
    {
        std::string names;
        ((names.append(names.empty() ? "" : separator).append(arc::typeName<Traits>)), ...);
        return names;
    }

    // How a link is spelt in the DSL, and where it is
    template<class Link>
    consteval std::string describeLink()
    {
        std::string text;
        if constexpr (requires { typename Link::TrunkFace; })
            text.append("[[").append(arc::typeName<typename Link::TrunkFace>).append(Link::fullUse ? "]]" : "*]]");
        else if constexpr (Link::isTrunk)
            text.append("[[").append(joinTraitNames(LinkTraits<Link>{}, " + ")).append("]]");
        else
            text.append("[").append(arc::typeName<typename Link::Trait>).append("]");
        return text.append(" at ").append(Link::Pos);
    }

    template<class... As, class... Bs>
    consteval std::string firstOverlap(TrunkTraitList<As...>, TrunkTraitList<Bs...>)
    {
        std::string found;
        ([&]<class A>()
        {
            ((found.empty() and TraitsOverlap<A, Bs> ? void(found = arc::typeName<A>) : void()), ...);
        }.template operator()<As>(), ...);
        return found;
    }

    // The first trait two links both carry, when either is a trunk, with both links; empty if there is none.
    // A link for a joined trait and a link for one of its traits do not overlap here: the node can name either.
    template<class LinkA, class LinkB>
    consteval std::string linkPairOverlap()
    {
        if constexpr (not LinkA::isTrunk and not LinkB::isTrunk)
        {
            return {};
        }
        else
        {
            std::string trait = firstOverlap(LinkTraits<LinkA>{}, LinkTraits<LinkB>{});
            if (trait.empty())
                return trait;
            return trait.append(" through ").append(describeLink<LinkA>()).append(" and ").append(describeLink<LinkB>());
        }
    }

    template<class Link, class... Rest>
    consteval std::string trunkLinksOverlap()
    {
        std::string report;
        ((report.empty() ? void(report = linkPairOverlap<Link, Rest>()) : void()), ...);
        if constexpr (sizeof...(Rest) > 0)
        {
            if (report.empty())
                report = trunkLinksOverlap<Rest...>();
        }
        return report;
    }

    // The first trait two links both carry, when either is a trunk, as a type: the failed requirement of the overlap
    // check names it, which is all a diagnostic shows where static_assert takes no computed message (before C++26)
    template<class Trait, class LinkA, class LinkB>
    struct TraitReachesNodeThroughTwoLinks
    {};

    struct NoTraitReachesNodeThroughTwoLinks
    {};

    template<class Overlap>
    inline constexpr bool noTraitReachesNodeThroughTwoLinks = std::is_same_v<Overlap, NoTraitReachesNodeThroughTwoLinks>;

    template<class A, class... Bs>
    consteval bool overlapsAnyOf(TrunkTraitList<Bs...>)
    {
        return (... or TraitsOverlap<A, Bs>);
    }

    // The index in LinkA's traits of the first that LinkB also carries, when either is a trunk, else -1
    template<class LinkA, class LinkB>
    consteval int linkPairOverlapIndex()
    {
        if constexpr (not LinkA::isTrunk and not LinkB::isTrunk)
        {
            return -1;
        }
        else
        {
            return []<class... As>(TrunkTraitList<As...>)
            {
                int index = -1;
                int i = 0;
                ((index < 0 and overlapsAnyOf<As>(LinkTraits<LinkB>{}) ? void(index = i) : void(), ++i), ...);
                return index;
            }(LinkTraits<LinkA>{});
        }
    }

    struct TrunkOverlapAt
    {
        bool found = false;
        std::size_t first = 0;
        std::size_t second = 0;
        int trait = -1;
    };

    template<std::size_t I, class Link, class... Rest>
    consteval TrunkOverlapAt firstTrunkOverlapFrom()
    {
        TrunkOverlapAt at;
        std::size_t j = I + 1;
        ((at.found or linkPairOverlapIndex<Link, Rest>() < 0
            ? void()
            : void(at = TrunkOverlapAt{true, I, j, linkPairOverlapIndex<Link, Rest>()}), ++j), ...);
        if constexpr (sizeof...(Rest) > 0)
        {
            if (not at.found)
                return firstTrunkOverlapFrom<I + 1, Rest...>();
        }
        return at;
    }

    template<std::size_t K, class... Traits>
    auto traitAt(TrunkTraitList<Traits...>) -> TypeAt<K, Traits...>;

    template<TrunkOverlapAt At, class... Links>
    struct TrunkOverlapOfAt
    {
        using type = NoTraitReachesNodeThroughTwoLinks;
    };

    template<TrunkOverlapAt At, class... Links>
    requires (At.found)
    struct TrunkOverlapOfAt<At, Links...>
    {
        using LinkA = TypeAt<At.first, Links...>;
        using type = TraitReachesNodeThroughTwoLinks<
            decltype(traitAt<static_cast<std::size_t>(At.trait)>(LinkTraits<LinkA>{})), LinkA, TypeAt<At.second, Links...>>;
    };

    template<class... Links>
    using TrunkOverlapOf = TrunkOverlapOfAt<firstTrunkOverlapFrom<0, Links...>(), Links...>::type;

    // The node a collection holds, whose dependencies are the collection's; any other node is its own
    template<class Node>
    using ElementNodeOf = GetNodeTypeToInstantiate<Node>;

    template<class Node>
    concept IsNodeCollection = not std::same_as<ElementNodeOf<Node>, NodeOf<Node>>;

    template<class Node>
    consteval bool declaresDependencyItems();

    template<class Item>
    struct ItemDeclared
    {
        static constexpr bool value = true;
    };

    template<class Node>
    struct ItemDeclared<UsesOf<Node>>
    {
        static constexpr bool value = declaresDependencyItems<Node>();
    };

    template<class... Items>
    consteval bool itemsDeclared(DependsItems<Items...>)
    {
        return (... and ItemDeclared<Items>::value);
    }

    // Whether a node lists its dependencies, which a full trunk is checked against: a wrapper lists those of the
    // nodes it wraps, and a collection its element's
    template<class Node>
    consteval bool declaresDependencyItems()
    {
        if constexpr (requires { typename Node::Depends::Items; })
            return itemsDeclared(typename Node::Depends::Items{});
        else if constexpr (IsNodeCollection<Node>)
            return declaresDependencyItems<ElementNodeOf<Node>>();
        else
            return false;
    }

    template<class Node, class Trait>
    consteval bool nodeUsesTrait();

    template<class Trait, class... Members>
    consteval bool trunkUses(Trunk<Members...> const*)
    {
        return (... or MatchesTrait<Members, Trait>);
    }

    template<class Trait, class Linked, class Asked, class ClientContext, bool Partial>
    consteval bool parentUseUses();

    template<class Trait, class Dependency>
    struct DependencyUses
    {
        static consteval bool get()
        {
            if constexpr (IsGlobalTrait<Dependency>)
                return false;
            else if constexpr (IsTrunk<Dependency>)
                return trunkUses<Trait>(static_cast<Dependency const*>(nullptr));
            else
                return MatchesTrait<Dependency, Trait>;
        }
    };

    template<class Trait, class Node>
    struct DependencyUses<Trait, UsesOf<Node>>
    {
        static consteval bool get() { return nodeUsesTrait<Node, Trait>(); }
    };

    template<class Trait, class Linked, class Asked, class ClientContext, bool Partial>
    struct DependencyUses<Trait, ParentUse<Linked, Asked, ClientContext, Partial>>
    {
        static consteval bool get() { return parentUseUses<Trait, Linked, Asked, ClientContext, Partial>(); }
    };

    template<class Trait, class Linked, class Asked, bool Partial>
    struct DependencyUses<Trait, ParentUse<Linked, Asked, void, Partial>>
    {
        static consteval bool get() { return false; }
    };

    // Whether a request for a dependency listed as Item resolves through a link for Trait. A trunk dependency
    // stands for each of its traits, a global one for none, a wrapped node's for what it uses, and a link a
    // cluster's node takes from the cluster's parent for what that node uses of it.
    template<class Trait, class... Items>
    consteval bool itemsUse(DependsItems<Items...>)
    {
        return (... or DependencyUses<Trait, std::remove_pointer_t<Items>>::get());
    }

    // Of a link for Linked that a cluster's node takes from the cluster's parent as Asked, whether Trait is used:
    // through each of Linked's traits that Trait resolves through and that the node uses, or, where the link renames
    // the trait, through Linked when the node uses what it asks for. A node that lists no dependencies is taken to
    // use all that its link carries, unless it took the link with `[[X*]]`.
    template<class Trait, class Linked, class Asked, class ClientContext, bool Partial>
    consteval bool parentUseUses()
    {
        using Client = ContextToNodeState<ClientContext>;
        return []<class... Members>(TrunkTraitList<Members...>)
        {
            if constexpr (not declaresDependencyItems<Client>())
                return not Partial and (... or MatchesTrait<Members, Trait>);
            else if constexpr (std::same_as<Linked, Asked>)
                return (... or (MatchesTrait<Members, Trait> and nodeUsesTrait<Client, Members>()));
            else
                return (... or MatchesTrait<Members, Trait>) and nodeUsesTrait<Client, Asked>();
        }(typename LinkTraitsOf<Linked>::type{});
    }

    template<class Node, class Trait>
    consteval bool nodeUsesTrait()
    {
        if constexpr (requires { typename Node::Depends::Items; })
            return itemsUse<Trait>(typename Node::Depends::Items{});
        else if constexpr (IsNodeCollection<Node>)
            return nodeUsesTrait<ElementNodeOf<Node>, Trait>();
        else
            return false;
    }

    // Whether the node uses every trait of a trunk
    template<class Node, class... Traits>
    consteval bool usesAllTrunkTraits(TrunkTraitList<Traits...>)
    {
        return (... and nodeUsesTrait<Node, Traits>());
    }

    // The traits of a trunk that the node does not use, comma separated
    template<class Node, class... Traits>
    consteval std::string unusedTrunkTraits(TrunkTraitList<Traits...>)
    {
        std::string unused;
        ((nodeUsesTrait<Node, Traits>() ? void() : void(unused.append(unused.empty() ? "" : ", ").append(arc::typeName<Traits>))), ...);
        return unused;
    }

    template<class Face>
    consteval std::string_view trunkDeclaration()
    {
        if constexpr (requires { { Face::SourceLocation } -> std::convertible_to<std::string_view>; })
            return Face::SourceLocation;
        else
            return "<unknown>";
    }

    template<class Node, class Link>
    consteval void assertTrunkUsedInFull()
    {
        if constexpr (Link::fullUse)
        {
            using Face [[maybe_unused]] = Link::TrunkFace;
            if constexpr (not declaresDependencyItems<Node>())
                ARC_STATIC_ASSERT_INLINE(declaresDependencyItems<Node>())(
                    "Node takes a trunk in full but lists no dependencies to check its use against: "
                    "list them with arc::Node::Uses<> or arc::Depends<>, or take only part of the trunk with [[...*]]",
                    (trunk, arc::typeName<Face>) \
                    (link, Link::Pos) \
                    ARC_SA_NODE_CONTEXT(node, Node));
            else
                ARC_STATIC_ASSERT_INLINE(usesAllTrunkTraits<Node>(LinkTraits<Link>{}))(
                "Node takes a trunk without using all of its traits: declare a trunk on the cluster for just the traits "
                "the node uses, `Trunk Name = ...`, and take it with [[cluster::X::Name]], or take those traits one link "
                "each; [[cluster::X*]] accepts a subset, for a one-off client only",
                (trunk, arc::typeName<Face>) \
                (unused, unusedTrunkTraits<Node>(LinkTraits<Link>{})) \
                (link, Link::Pos) \
                (declared, trunkDeclaration<Face>()) \
                ARC_SA_NODE_CONTEXT(node, Node));
        }
    }

    // For the links of a cluster's parent into the cluster, as no node holds them
    ARC_MODULE_EXPORT
    template<class... Links>
    consteval bool assertParentTrunkLinksDisjoint()
    {
        ARC_STATIC_ASSERT_INLINE(noTraitReachesNodeThroughTwoLinks<TrunkOverlapOf<Links...>>)(
            "The cluster takes a trait from its parent through a trunk and through another link, and a trunk cannot "
            "be disambiguated inside the cluster: take the trait through one link only",
            (overlap, trunkLinksOverlap<Links...>()));
        return true;
    }

} // namespace detail

namespace pred {

    // Checks the links a node holds, when one of them is a trunk: that no trait reaches the node through a trunk and
    // another link, and that the node uses every trait of each trunk it takes in full
    ARC_MODULE_EXPORT
    template<class... Links>
    struct TrunkLinks
    {
        // Before the node's dependencies, as two links carrying a trait leave the trait unresolved
        template<class Node>
        static consteval bool assertBeforeDependencies()
        {
            ARC_STATIC_ASSERT_INLINE(detail::noTraitReachesNodeThroughTwoLinks<detail::TrunkOverlapOf<Links...>>)(
                "Node takes a trait through a trunk and through another link, and a trunk cannot be disambiguated "
                "inside the node: take the trait through one link only",
                (overlap, detail::trunkLinksOverlap<Links...>()) \
                ARC_SA_NODE_CONTEXT(node, Node));
            return detail::noTraitReachesNodeThroughTwoLinks<detail::TrunkOverlapOf<Links...>>;
        }

        template<class Node>
        static consteval void assert()
        {
            (detail::assertTrunkUsedInFull<Node, Links>(), ...);
        }
    };

} // namespace pred

} // namespace arc

#endif // INCLUDE_ARC_TRUNK_LINKS_HPP
