#ifndef INCLUDE_ARC_STATIC_ASSERT_MACROS_HPP
#define INCLUDE_ARC_STATIC_ASSERT_MACROS_HPP

#include "arc/macros.hpp"

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

// Asserts a requirement of Trait on its implementation Impl with Types, for use in arc::assertImplements checks
// ARC_ASSERT_IMPLEMENTS(Trait, Impl, Types, "message", requirement...)
#define ARC_ASSERT_IMPLEMENTS(Trait, Impl, Types, message, ...) \
    ARC_ASSERT_IMPLEMENTS_KV(Trait, Impl, Types, message, , __VA_ARGS__)

// As ARC_ASSERT_IMPLEMENTS, with extra key-value pairs printed after the node's
// ARC_ASSERT_IMPLEMENTS_KV(Trait, Impl, Types, "message", (key1, value1)(key2, value2)..., requirement...)
#define ARC_ASSERT_IMPLEMENTS_KV(Trait, Impl, Types, message, kvs, ...) \
    ARC_STATIC_ASSERT_INLINE(__VA_ARGS__)(message, \
        (trait, ::arc::typeNameDecompressed<Trait>) \
        (types, ::arc::typeNameDecompressed<Types>) \
        (impl, ::arc::typeNameDecompressed<Impl>) \
        ARC_SA_NODE_CONTEXT(node, Impl) kvs)

// Asserts that Impl implements a trait method, telling apart a method that is missing from one that is not const or
// returns the wrong type. Requirement completes the name of the arc::ImplementsMethod* concept asserted, as
// ImplementsMethod##Requirement<Receiver, Method, Return, Params...>, so that a failure also gets the compiler's
// concept diagnostics: AnyReturn, ConvertibleTo or Matching. Pasted on in the macro body, so that compilers report
// the failed condition where the macro is invoked (GCC would otherwise point at the argument).
// The generic lambda names the types it deduces from the probe, so diagnostics print the requirement by those names
// rather than as spelled at the call (where dependent types are wrapped in ARC_TYPENAME).
// ARC_ASSERT_IMPLEMENTS_METHOD(Trait, methodName, Impl, Types, "file:line:col: ", "signature", Requirement, IsConst, Return, Params...)
#define ARC_ASSERT_IMPLEMENTS_METHOD(Trait, methodName, Impl, Types, pos, signature, Requirement, IsConst, ...) \
    []<class ARC_Impl, bool ARC_IsConst, class ARC_Method, class ARC_Return, class... ARC_Params>( \
        ::arc::MethodProbe<ARC_Impl, ARC_IsConst, ARC_Method, ARC_Return, ARC_Params...> ARC_probe) \
    { \
        using ARC_Probe = decltype(ARC_probe); \
        using ARC_Receiver = ARC_Probe::Receiver; \
        if constexpr (not ::arc::ImplementsMethod##Requirement<ARC_Receiver, ARC_Method, ARC_Return, ARC_Params...>) \
        { \
            if constexpr (ARC_Probe::needsConst()) \
                ARC_ASSERT_IMPLEMENTS(Trait, Impl, Types, pos "Trait method must be const: " signature, \
                    ::arc::ImplementsMethod##Requirement<ARC_Receiver, ARC_Method, ARC_Return, ARC_Params...>); \
            else if constexpr (ARC_Probe::callable()) \
                ARC_ASSERT_IMPLEMENTS_KV(Trait, Impl, Types, pos "Trait method return type does not match the trait's: " signature, \
                    (actual_return, ::arc::typeName<typename ARC_Probe::Result>), \
                    ::arc::ImplementsMethod##Requirement<ARC_Receiver, ARC_Method, ARC_Return, ARC_Params...>); \
            else \
                ARC_ASSERT_IMPLEMENTS(Trait, Impl, Types, pos "Trait method not implemented: " signature, \
                    ::arc::ImplementsMethod##Requirement<ARC_Receiver, ARC_Method, ARC_Return, ARC_Params...>); \
        } \
    }(::arc::MethodProbe<Impl, IsConst, Trait::methodName, __VA_ARGS__>{})

#endif // INCLUDE_ARC_STATIC_ASSERT_MACROS_HPP
