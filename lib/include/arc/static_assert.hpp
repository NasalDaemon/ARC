#ifndef INCLUDE_ARC_STATIC_ASSERT_HPP
#define INCLUDE_ARC_STATIC_ASSERT_HPP

#include "arc/detail/compress.hpp"
#include "arc/compiler.hpp"
#include "arc/context_fwd.hpp"
#include "arc/macros.hpp"
#include "arc/static_assert_macros.hpp"
#include "arc/type_name.hpp"

#if !ARC_IMPORT_STD
#include <concepts>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#endif

namespace arc {

namespace detail {
    // Decimal representation of an integer
    template<std::integral T>
    consteval std::string numberString(T n)
    {
        if constexpr (std::is_same_v<T, bool>)
        {
            return n ? "true" : "false";
        }
        else
        {
            bool const negative = n < 0;
            std::string result;
            do
            {
                auto const digit = n % 10;
                result.insert(result.begin(), char('0' + (negative ? -digit : digit)));
                n /= 10;
            } while (n != 0);
            if (negative)
                result.insert(result.begin(), '-');
            return result;
        }
    }

    // Converts an ARC_STATIC_ASSERT key-value pair's value to a string
    // Exported, along with the other functions used by the ARC_STATIC_ASSERT macros, for generated module units
    ARC_MODULE_EXPORT
    template<class T>
    consteval std::string staticAssertString(T const& value)
    {
        if constexpr (std::is_same_v<T, bool> or std::integral<T>)
            return numberString(value);
        else if constexpr (std::is_enum_v<T>)
            return numberString(std::to_underlying(value));
        else if constexpr (std::is_constructible_v<std::string, T const&>)
            return std::string(value);
        else
            static_assert(false, "ARC_STATIC_ASSERT value must be an integer, enum or string");
    }

    // Declared by contexts that wrap another context without being a named member of a cluster
    // (e.g. arc::Union/arc::Virtual inner contexts), so that their path is that of the wrapped context.
    // Element contexts of collections append "[]" to the path.
    template<class Context_, bool Element = false>
    struct NodePathOf
    {
        using Context = Context_;
        static constexpr bool IsElement = Element;
    };

    // Preferred: declared in the context itself, so it is available before the parent cluster is complete
    template<class C>
    concept HasNodeNameMember = requires { { C::NodeName } -> std::convertible_to<char const*>; };

    // Fallback: declared by ARC_NODE in the parent cluster
    template<class C>
    concept HasNodeName = requires { { getNodeName(AdlTag<C>{}) } -> std::convertible_to<char const*>; };
}

// Path of the node from the root of the graph, made of the node names in each cluster, e.g. ".cluster.node"
ARC_MODULE_EXPORT
template<IsContext C_>
consteval std::string nodePath()
{
    using C = detail::Decompress<C_>;
    if constexpr (requires { typename C::NodePath::Context; })
    {
        using Wrapped = C::NodePath;
        return nodePath<typename Wrapped::Context>().append(Wrapped::IsElement ? "[]" : "");
    }
    else if constexpr (not requires { typename C::ParentContext; })
    {
        return ""; // root
    }
    else
    {
        using P = detail::Decompress<typename C::ParentContext>;
        // Transparent contexts, such as the inner node of a wrapper cluster, share the depth of their parent
        if constexpr (C::Depth == P::Depth)
            return nodePath<P>();
        else if constexpr (detail::HasNodeNameMember<C>)
            return nodePath<P>().append(".").append(C::NodeName);
        else if constexpr (detail::HasNodeName<C>)
            return nodePath<P>().append(".").append(getNodeName(AdlTag<C>{}));
        else
            return nodePath<P>().append(".<?>");
    }
}

namespace detail {
    // The context that declares the node as a member of its cluster, skipping wrapper and transparent contexts
    template<class C_>
    consteval auto declaringContext()
    {
        using C = Decompress<C_>;
        if constexpr (requires { typename C::NodePath::Context; })
            return declaringContext<typename C::NodePath::Context>();
        else if constexpr (not requires { typename C::ParentContext; })
            return std::type_identity<C>{}; // root
        else if constexpr (C::Depth == Decompress<typename C::ParentContext>::Depth)
            return declaringContext<typename C::ParentContext>();
        else
            return std::type_identity<C>{};
    }

    template<class T>
    consteval auto staticAssertContext()
    {
        using U = std::remove_cvref_t<T>;
        if constexpr (IsContext<U>)
            return std::type_identity<U>{};
        else if constexpr (requires { requires IsContext<ContextOf<U>>; })
            return std::type_identity<ContextOf<U>>{};
        else
            return std::type_identity<void>{};
    }

    // For ARC_SA_NODE: inner node handle of a context or node, falling back to the type itself
    ARC_MODULE_EXPORT
    template<class T>
    consteval std::string_view staticAssertNodeName()
    {
        using C = decltype(staticAssertContext<T>())::type;
        if constexpr (std::is_void_v<C>)
            return arc::typeName<T>;
        else if constexpr (std::is_void_v<InnerNodeHandle<C>>)
            return arc::typeName<C>;
        else
            return arc::typeName<InnerNodeHandle<C>>;
    }

    // For ARC_SA_NODE_CONTEXT: path of a context or node, if it has one
    ARC_MODULE_EXPORT
    template<class T>
    consteval std::string staticAssertNodePath()
    {
        using C = decltype(staticAssertContext<T>())::type;
        if constexpr (std::is_void_v<C>)
            return "<none>";
        else
            return nodePath<C>();
    }

    // Inner node handle of the cluster a context is the context of, including the root cluster when known
    template<class C>
    consteval auto clusterHandleOf()
    {
        if constexpr (requires { typename C::RootCluster; })
            return std::type_identity<typename C::RootCluster>{};
        else
            return std::type_identity<InnerNodeHandle<C>>{};
    }

    // For ARC_SA_NODE_CONTEXT: inner node handle of the cluster that declares the node, and the file the DSL cluster is in
    ARC_MODULE_EXPORT
    template<class T>
    consteval std::string staticAssertParentCluster()
    {
        using C = decltype(staticAssertContext<T>())::type;
        if constexpr (std::is_void_v<C>)
        {
            return "<none>";
        }
        else
        {
            using Declaring = decltype(declaringContext<C>())::type;
            if constexpr (not requires { typename Declaring::ParentContext; })
            {
                return "<root>";
            }
            else
            {
                using P = Decompress<typename Declaring::ParentContext>;
                using Cluster = decltype(clusterHandleOf<P>())::type;
                if constexpr (std::is_void_v<Cluster>)
                    // Root context without the cluster's handle (i.e. not created by arc::Graph)
                    return std::string(arc::typeName<typename Declaring::Parent>);
                else if constexpr (requires { { Cluster::SourceLocation } -> std::convertible_to<std::string_view>; })
                    return std::string(arc::typeName<Cluster>).append(" (").append(Cluster::SourceLocation).append(")");
                else
                    return std::string(arc::typeName<Cluster>);
            }
        }
    }

    // Comma separated names of the node's unsatisfied dependencies
    template<class Node, bool Transitive>
    consteval std::string missingDependencies()
    {
        if constexpr (requires { Node::Depends::template missingDependencies<Node, Transitive>(); })
            return Node::Depends::template missingDependencies<Node, Transitive>();
        else
            return "<unknown>";
    }
}

// Asserts that Impl, with Types, implements Trait. Called whenever a TraitView of Impl is made, e.g. by getNode and asTrait.
// DSL traits assert each of their requirements independently, naming the requirement as written in the DSL.
ARC_MODULE_EXPORT
template<class Impl, class Types, class Trait>
consteval void assertImplements()
{
    Trait::Meta::template assertImplements<Trait, detail::Decompress<Impl>, detail::Decompress<Types>>();
}

} // namespace arc

#endif // INCLUDE_ARC_STATIC_ASSERT_HPP
