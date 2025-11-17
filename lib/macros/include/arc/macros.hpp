#ifndef INCLUDE_ARC_MACROS_HPP
#define INCLUDE_ARC_MACROS_HPP

#ifndef ARC_MODULE_EXPORT
#   define ARC_MODULE_EXPORT
#endif

#define ARC_CAT(l, r) l ## r
#define ARC_JOIN(l, r) l r
#define ARC_APPLY(macro, ...) ARC_JOIN(macro, (__VA_ARGS__))
#define ARC_DEPAREN(X) ARC_ESC(ARC_ISH X)
#define ARC_ISH(...) ARC_ISH __VA_ARGS__
#define ARC_ESC(...) ARC_ESC_(__VA_ARGS__)
#define ARC_ESC_(...) ARC_VANISH_ ## __VA_ARGS__
#define ARC_VANISH_ARC_ISH

// Count number of arguments
#define ARC_ARGC(...) ARC_APPLY( \
    ARC_ARGC_,__VA_ARGS__, 63,62,61,60, \
        59,58,57,56,55,54,53,52,51,50, \
        49,48,47,46,45,44,43,42,41,40, \
        39,38,37,36,35,34,33,32,31,30, \
        29,28,27,26,25,24,23,22,21,20, \
        19,18,17,16,15,14,13,12,11,10, \
         9, 8, 7, 6, 5, 4, 3, 2, 1, 0)
#define ARC_ARGC_( \
     _1, _2, _3, _4, _5, _6, _7, _8, _9,_10, \
    _11,_12,_13,_14,_15,_16,_17,_18,_19,_20, \
    _21,_22,_23,_24,_25,_26,_27,_28,_29,_30, \
    _31,_32,_33,_34,_35,_36,_37,_38,_39,_40, \
    _41,_42,_43,_44,_45,_46,_47,_48,_49,_50, \
    _51,_52,_53,_54,_55,_56,_57,_58,_59,_60, \
    _61,_62,_63,N,...) N

#define ARC_OVERLOAD(macro, ...) ARC_APPLY(ARC_CAT, macro, ARC_ARGC(__VA_ARGS__))

// ARC_MAKE_VERSION(major, minor=0, patch=0)
#define ARC_MAKE_VERSION(...) ARC_OVERLOAD(ARC_MAKE_VERSION, __VA_ARGS__)(__VA_ARGS__)

// MMM|NNN|PPPPP
#define ARC_MAKE_VERSION3(major, minor, patch) ((1000ull * 100000ull * (major)) + (100000ull * (minor)) + (patch))
#define ARC_MAKE_VERSION2(major, minor)        ARC_MAKE_VERSION3(major, minor, 0)
#define ARC_MAKE_VERSION1(major)               ARC_MAKE_VERSION3(major, 0, 0)

// Prefer using C++ wrappers in arc::compiler
#if defined(__clang__)
#   define ARC_COMPILER_IS_X(clang, ...) ARC_DEPAREN(clang)
#   define ARC_COMPILER_VERSION ARC_MAKE_VERSION(__clang_major__, __clang_minor__, __clang_patchlevel__)
#elif defined(__GNUC__) || defined(__GNUG__)
#   define ARC_COMPILER_IS_X(clang, gcc, ...) ARC_DEPAREN(gcc)
#   define ARC_COMPILER_VERSION ARC_MAKE_VERSION(__GNUC__, __GNUC_MINOR__, __GNUC_PATCHLEVEL__)
#elif defined(_MSC_VER)
#   define ARC_COMPILER_IS_X(clang, gcc, msvc, ...) ARC_DEPAREN(msvc)
// _MSC_VER MM|NN
// _MSC_FULL_VER MM|NN|PPPPP
#   define ARC_COMPILER_VERSION ARC_MAKE_VERSION(_MSC_VER / 100, _MSC_VER % 100, _MSC_FULL_VER % 100'000)
#else
#   error Unsupported compiler
#endif

//                                              CLANG          GCC            MSVC
#define ARC_IF_CLANG(...)      ARC_COMPILER_IS_X((__VA_ARGS__), (           ), (           ))
#define ARC_IF_GCC(...)        ARC_COMPILER_IS_X((           ), (__VA_ARGS__), (           ))
#define ARC_IF_GNU(...)        ARC_COMPILER_IS_X((__VA_ARGS__), (__VA_ARGS__), (           ))
#define ARC_IF_MSVC(...)       ARC_COMPILER_IS_X((           ), (           ), (__VA_ARGS__))
#define ARC_IF_NOT_CLANG(...)  ARC_COMPILER_IS_X((           ), (__VA_ARGS__), (__VA_ARGS__))
#define ARC_IF_NOT_GCC(...)    ARC_COMPILER_IS_X((__VA_ARGS__), (           ), (__VA_ARGS__))
#define ARC_IF_NOT_GNU(...)    ARC_COMPILER_IS_X((           ), (           ), (__VA_ARGS__))
#define ARC_IF_NOT_MSVC(...)   ARC_COMPILER_IS_X((__VA_ARGS__), (__VA_ARGS__), (           ))

// Calling convention: ARC_IF_CLANG_ELSE(IF_CLANG...)(ELSE...)
#define ARC_IF_CLANG_ELSE(...)  ARC_IF_CLANG(__VA_ARGS__) ARC_IF_NOT_CLANG
#define ARC_IF_GCC_ELSE(...)    ARC_IF_GCC  (__VA_ARGS__) ARC_IF_NOT_GCC
#define ARC_IF_GNU_ELSE(...)    ARC_IF_GNU  (__VA_ARGS__) ARC_IF_NOT_GNU
#define ARC_IF_MSVC_ELSE(...)   ARC_IF_MSVC (__VA_ARGS__) ARC_IF_NOT_MSVC

#define ARC_COMPILER_CLANG ARC_IF_CLANG_ELSE(1)(0)
#define ARC_COMPILER_GCC   ARC_IF_GCC_ELSE(1)(0)
#define ARC_COMPILER_GNU   ARC_IF_GNU_ELSE(1)(0)
#define ARC_COMPILER_MSVC  ARC_IF_MSVC_ELSE(1)(0)

// ARC_COMPILER_GE(GCC, major, minor=0, patch=0)
#define ARC_COMPILER_GE(NAME, ...) ARC_COMPILER_CMP(NAME, >=, __VA_ARGS__)
// ARC_COMPILER_LT(GCC, major, minor=0, patch=0)
#define ARC_COMPILER_LT(NAME, ...) ARC_COMPILER_CMP(NAME, <, __VA_ARGS__)

#define ARC_COMPILER_CMP(NAME, op, ...) (ARC_CAT(ARC_COMPILER_, NAME) && (ARC_COMPILER_VERSION op ARC_MAKE_VERSION(__VA_ARGS__)))

#if ARC_COMPILER_CLANG
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wgnu-zero-variadic-macro-arguments"
#endif

#if ARC_COMPILER_MSVC
#   define ARC_CPP_VERSION _MSVC_LANG
#else
#   define ARC_CPP_VERSION __cplusplus
#endif

#if defined(_WIN32)
#   define ARC_OS_WINDOWS 1
#else
#   define ARC_OS_WINDOWS 0
#endif

#if defined(__INTELLISENSE__) or defined(ARC_CLANGD)
#   define ARC_AUTOCOMPLETE 1
#else
#   define ARC_AUTOCOMPLETE 0
#endif

#if ARC_COMPILER_MSVC
#   define ARC_INLINE [[msvc::forceinline]] inline
#else
#   define ARC_INLINE [[gnu::always_inline, gnu::artificial]] inline
#endif

#define ARC_COLD [[using ARC_IF_GNU_ELSE(gnu)(msvc): noinline, cold]]

// Clang won't detail the failure in the build diagnostic when asserting the concept directly
#if ARC_COMPILER_CLANG
#   define ARC_ASSERT_IMPLEMENTS(Impl, Types, Trait) \
    static_assert(::arc::detail::alwaysTrue<::arc::detail::Implements<Impl, Types, Trait>>)
#else
#   define ARC_ASSERT_IMPLEMENTS(Impl, Types, Trait) \
    static_assert(::arc::Implements<Impl, Types, Trait>)
#endif

#define ARC_FWD(...) ARC_OVERLOAD(ARC_FWD, __VA_ARGS__)(__VA_ARGS__)

#define ARC_FWD1(name)    ARC_FWD2(decltype(name), name)
#define ARC_FWD2(T, name) static_cast<T&&>(name)

// ARC_METHODS(TraitName, MethodList=ARC_METHODS_TraitName)
#define ARC_METHODS(...) ARC_OVERLOAD(ARC_METHODS, __VA_ARGS__)(__VA_ARGS__)

#define ARC_METHODS1(traitName) \
    ARC_METHODS2(traitName, ARC_METHODS_ ## traitName)

#define ARC_METHODS2(traitName, METHOD_LIST) \
    METHOD_LIST(ARC_METHOD_TAG) \
    friend constexpr traitName traitOf(::arc::IsMethodOf<traitName> auto) { return {}; } \
    struct Meta \
    { \
        struct Applicable \
        { \
            METHOD_LIST(ARC_METHOD_TAG_APPLICABLE) \
        }; \
        struct Methods \
        { \
            METHOD_LIST(ARC_DUCK_METHOD) \
        }; \
        using DuckMethods = Methods; \
    };

#define ARC_METHOD_TAG(method) \
    struct method{} static constexpr method ## _c{};

#define ARC_METHOD_TAG_APPLICABLE(method) \
    static void applicable(method);

#define ARC_AS_FUNCTOR_METHOD(method) \
    template<::arc::IsTraitView Self> \
    ARC_INLINE constexpr decltype(auto) method(this Self&& self, ::arc::AsFunctor asFunctor) \
    { \
        return self.impl(method ## _c, asFunctor); \
    }

#define ARC_DUCK_METHOD(method) \
    template<::arc::IsTraitView Self> \
    ARC_INLINE constexpr decltype(auto) method(this Self&& self, auto&&... args) \
    { \
        return self.impl(method ## _c, ARC_FWD(args)...); \
    }

// ARC_LINK(TraitName, TargetContext, TargetTraitRename=<NoRename>)
#define ARC_LINK(...) ARC_OVERLOAD(ARC_LINK, __VA_ARGS__)(__VA_ARGS__)

#define ARC_LINK2(traitName, targetContext) \
    ARC_LINK3(traitName, targetContext, T)

#define ARC_LINK3(traitName, targetContext, targetTrait) \
    template<::arc::MatchesTrait<ARC_DEPAREN(traitName)> T> \
    static ::arc::ResolvedLink<ARC_DEPAREN(targetContext), ARC_DEPAREN(targetTrait)> resolveLink(T);

#define ARC_NODE(Context, nodeName, ... /* predicates */) \
    [[no_unique_address]] ::arc::Ensure<::arc::ContextToNodeState<Context>, ## __VA_ARGS__> nodeName{}; \
    friend consteval auto getNodePointer(arc::AdlTag<Context>) \
    { \
        using P = Context::Parent; \
        return ::arc::detail::memberPtr<P>(&P::nodeName); \
    }

#define ARC_INSTANTIATE(graph, dotPath) \
    template struct std::remove_pointer_t<std::remove_cvref_t<decltype(ARC_DEPAREN(graph)::dotPath)>>::Node< \
        ::arc::ContextOf<std::remove_pointer_t<std::remove_cvref_t<decltype(ARC_DEPAREN(graph)::dotPath)>>>>;

#define ARC_INSTANTIATE_BOX(Main, InFacade, ... /* interfaces */) \
    ARC_INSTANTIATE((::arc::Box<Main, InFacade, ## __VA_ARGS__>::Graph), main)

// used in main.cpp
#define ARC_CONSTRUCT(...) \
    ::arc::constructGraph([&]() -> decltype(auto) \
    { \
        return (__VA_ARGS__); \
    })

#define ARC_EMPLACE(...) \
    ::arc::Emplace([&](auto t) { return typename decltype(t)::type{__VA_ARGS__}; })

#if ARC_COMPILER_CLANG
#pragma clang diagnostic pop
#endif

#endif // INCLUDE_ARC_MACROS_HPP
