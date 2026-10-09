#ifndef INCLUDE_ARC_STATIC_ASSERT_HPP
#define INCLUDE_ARC_STATIC_ASSERT_HPP

#include "arc/detail/compress.hpp"
#include "arc/compiler.hpp"
#include "arc/context_fwd.hpp"
#include "arc/macros.hpp"
#include "arc/type_name.hpp"

#if !ARC_IMPORT_STD
#include <concepts>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#endif

// ARC_STATIC_ASSERT(cond)(message, (key1, value1)(key2, value2)...)
// Leading newline so the art starts at column 0 after the compiler's own diagnostic prefix,
// plus a blank line before and after to separate it from surrounding diagnostics
// Clang escapes non-printable characters in diagnostics (ESC -> <U+001B>), so no ANSI colours there
#define ARC_SA_ERR   ARC_IF_NOT_CLANG("\033[1;91m")
#define ARC_SA_MSG   ARC_IF_NOT_CLANG("\033[93m")
#define ARC_SA_KEY   ARC_IF_NOT_CLANG("\033[1;96m")
#define ARC_SA_VAL   ARC_IF_NOT_CLANG("\033[32m")
#define ARC_SA_RESET ARC_IF_NOT_CLANG("\033[0m")
// start all lines with "error: " so all lines appear when grepping for errors
#define ARC_SA_PREFIX(padding) "\n" ARC_SA_ERR "error: " padding ARC_SA_RESET ARC_IF_MSVC_ELSE(" | ")(" \u2502 ")
#define ARC_SA_ART1 ARC_SA_PREFIX("╔═╗╦═╗╔═╗")
#define ARC_SA_ART2 ARC_SA_PREFIX("╠═╣╠╦╝║  ")
#define ARC_SA_ART3 ARC_SA_PREFIX("╩ ╩╩╚═╚═╝")
#define ARC_SA_PAD  ARC_SA_PREFIX("         ")

#if ARC_CPP_VERSION > 202302L and __cpp_static_assert >= 202306L
#define ARC_STATIC_ASSERT_CONTEXT 1

#define ARC_STATIC_ASSERT_FMT_STR(str, ...) \
    std::string("\n" ARC_SA_ART1 ARC_SA_MSG).append(str).append(ARC_SA_RESET ARC_SA_ART2) ARC_SA_LOOP_A __VA_ARGS__ ().append("\n")

#define ARC_STATIC_ASSERT_INLINE(...) \
    do { \
        if constexpr (!(__VA_ARGS__)) \
        { \
            static_assert(__VA_ARGS__, ARC_STATIC_ASSERT_INLINE_FMT
#define ARC_STATIC_ASSERT_INLINE_FMT(str, ...) \
                ARC_STATIC_ASSERT_FMT_STR(str, __VA_ARGS__)); \
        } \
    } while(0)

#if ARC_COMPILER_GCC
// GCC with modules does not support IILE in static_assert
#define ARC_STATIC_ASSERT(...) \
    static consteval void ARC_APPLY(ARC_CAT, _arcStaticAssert, __LINE__)() \
    { \
        if constexpr (!(__VA_ARGS__)) \
        { \
            static_assert(__VA_ARGS__, ARC_STATIC_ASSERT_FMT
#define ARC_STATIC_ASSERT_FMT(str, ...) \
                ARC_STATIC_ASSERT_FMT_STR(str, __VA_ARGS__)); \
        } \
    } \
    static_assert((ARC_APPLY(ARC_CAT, _arcStaticAssert, __LINE__)(), true))
#else
#define ARC_STATIC_ASSERT(...) \
    static_assert(([] { \
        if constexpr (!(__VA_ARGS__)) \
        { \
            static_assert(__VA_ARGS__, ARC_STATIC_ASSERT_FMT
#define ARC_STATIC_ASSERT_FMT(str, ...) \
                ARC_STATIC_ASSERT_FMT_STR(str, __VA_ARGS__)); \
        } \
    }(), true))
#endif

// Built with std::string::append rather than operator+: when arc is a module whose global module fragment includes the
// standard library headers, importers cannot reach the std::string operator+ overloads that the module does not use
#define ARC_SA_KV(a, ...) \
    .append(ARC_SA_KEY #a ARC_SA_RESET " = " ARC_SA_VAL).append(::arc::detail::staticAssertString(__VA_ARGS__))ARC_IF_NOT_CLANG(.append(ARC_SA_RESET))
// A: first pair beside art line 2, then always emit art line 3
#define ARC_SA_LOOP_A(...) \
    __VA_OPT__(ARC_APPLY(ARC_SA_KV, __VA_ARGS__)).append(ARC_SA_ART3) __VA_OPT__(ARC_SA_LOOP_B)
// B: second pair beside art line 3
#define ARC_SA_LOOP_B(...) \
    __VA_OPT__(ARC_APPLY(ARC_SA_KV, __VA_ARGS__) ARC_SA_LOOP_C)
// C/D: remaining pairs on their own lines, padded to the art width
#define ARC_SA_LOOP_C(...) \
    __VA_OPT__(.append(ARC_SA_PAD) ARC_APPLY(ARC_SA_KV, __VA_ARGS__) ARC_SA_LOOP_D)
#define ARC_SA_LOOP_D(...) \
    __VA_OPT__(.append(ARC_SA_PAD) ARC_APPLY(ARC_SA_KV, __VA_ARGS__) ARC_SA_LOOP_C)

#else
#define ARC_STATIC_ASSERT_CONTEXT 0

#define ARC_STATIC_ASSERT(...) static_assert(__VA_ARGS__, ARC_STATIC_ASSERT_FMT
#define ARC_STATIC_ASSERT_FMT(str, ...) \
    ARC_SA_ART1 ARC_SA_MSG str ARC_SA_RESET ARC_SA_ART2 ARC_SA_LOOP_A __VA_ARGS__ () \
    ARC_SA_PAD "note: context <?> only available when compiling with C++ 26+\n")
#define ARC_STATIC_ASSERT_INLINE ARC_STATIC_ASSERT
#define ARC_SA_KV(a, ...) \
    ARC_SA_KEY #a ARC_SA_RESET " = " ARC_SA_VAL "<?>" ARC_SA_RESET ""
// A: first pair beside art line 2, then always emit art line 3
#define ARC_SA_LOOP_A(...) \
    __VA_OPT__(ARC_APPLY(ARC_SA_KV, __VA_ARGS__)) ARC_SA_ART3 __VA_OPT__(ARC_SA_LOOP_B)
// B: second pair beside art line 3
#define ARC_SA_LOOP_B(...) \
    __VA_OPT__(ARC_APPLY(ARC_SA_KV, __VA_ARGS__) ARC_SA_LOOP_C)
// C/D: remaining pairs on their own lines, padded to the art width
#define ARC_SA_LOOP_C(...) \
    __VA_OPT__(ARC_SA_PAD ARC_APPLY(ARC_SA_KV, __VA_ARGS__) ARC_SA_LOOP_D)
#define ARC_SA_LOOP_D(...) \
    __VA_OPT__(ARC_SA_PAD ARC_APPLY(ARC_SA_KV, __VA_ARGS__) ARC_SA_LOOP_C)
#endif

// Key-value pairs describing a node, given its context or node type
// ARC_STATIC_ASSERT(cond)(message, ARC_SA_NODE(node, Context)) --> node = <inner node handle>
#define ARC_SA_NODE(key, ...) \
    (key, ::arc::detail::staticAssertNodeName<__VA_ARGS__>())
// For failures whose fix depends on where the node sits in the graph, also prints its path and declaring cluster
// ARC_SA_NODE_CONTEXT(node, Context) --> node = <inner node handle>, node_path = .cluster.node, node_cluster = <cluster inner node handle> in <file>
#define ARC_SA_NODE_CONTEXT(key, ...) \
    ARC_SA_NODE(key, __VA_ARGS__) \
    (key##_path, ::arc::detail::staticAssertNodePath<__VA_ARGS__>()) \
    (key##_cluster, ::arc::detail::staticAssertParentCluster<__VA_ARGS__>())

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
                else if constexpr (requires { { Cluster::FileName } -> std::convertible_to<std::string_view>; })
                    return std::string(arc::typeName<Cluster>).append(" (").append(Cluster::FileName).append(")");
                else
                    return std::string(arc::typeName<Cluster>);
            }
        }
    }

    template<class Impl, class Types, class Trait>
    consteval void assertImplements()
    {
        ARC_STATIC_ASSERT_INLINE(arc::Implements<Impl, Types, Trait>)(
            "Not all requirements of Trait are met by Impl/Types. See concept failure below.",
            (trait, arc::typeName<Trait>)
            (impl, arc::typeName<Decompress<Impl>>)
            (types, arc::typeName<Decompress<Types>>)
            ARC_SA_NODE_CONTEXT(node, Impl));
        // Clang won't detail which method is missing when asserting the concept, so also instantiate the failing requirement
        if constexpr (isClang)
            static_assert(alwaysTrue<detail::Implements<Impl, Types, Trait>>);
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

} // namespace arc

#endif // INCLUDE_ARC_STATIC_ASSERT_HPP
