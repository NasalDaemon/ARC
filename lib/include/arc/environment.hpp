#ifndef INCLUDE_ARC_ENVIRONMENT_HPP
#define INCLUDE_ARC_ENVIRONMENT_HPP

#include "arc/detail/cast.hpp"
#include "arc/empty_types.hpp"
#include "arc/macros.hpp"

#if !ARC_IMPORT_STD
#include <concepts>
#include <type_traits>
#endif

namespace arc {

ARC_MODULE_EXPORT
template<class E, class Target>
struct EnvironmentOverlay : Target
{
    using Environment = E;
};

ARC_MODULE_EXPORT
template<class T>
concept EnvironmentPart = requires
{
    typename T::Tag;
    requires IsStateless<typename T::Tag>;
    { T::resolveEnvironment(typename T::Tag{}) } -> std::same_as<T>;
    { T::isDynamic() } -> std::convertible_to<bool>;
    requires IsStateless<T>;
};

namespace detail {
    template<class Env, class... Es>
    struct InsertMissing;
    template<class Keep, class Rest>
    struct RemoveDynamic;
}

ARC_MODULE_EXPORT
template<EnvironmentPart... Es>
struct Environment : Es...
{
    static void resolveEnvironment();
    using Es::resolveEnvironment...;

    static constexpr bool IsEmpty = sizeof...(Es) == 0;

    template<class Tag>
    static constexpr bool HasTag = requires (Tag tag) { resolveEnvironment(tag); };

    template<class Tag>
    using Get = decltype(resolveEnvironment(Tag{}));

    template<class ToReplace, std::same_as<ToReplace> Old, class Replacement>
    static auto replaceEach(Old, Replacement) -> Replacement;

    template<class ToReplace, class Old, class Replacement>
    static auto replaceEach(Old, Replacement) -> Old;

    template<EnvironmentPart E>
    using Replace = Environment<decltype(replaceEach<Get<typename E::Tag>>(Es{}, E{}))...>;

    template<class E>
    requires HasTag<typename E::Tag>
    static auto insertOrReplace() -> Replace<E>;

    template<class E>
    static auto insertOrReplace() -> Environment<Es..., E>;

    template<class E>
    using InsertOrReplace = decltype(insertOrReplace<E>());

    template<class... E2s>
    using InsertMissing = detail::InsertMissing<Environment, E2s...>::type;

    template<class... E2s>
    using Merge = detail::InsertMissing<Environment<E2s...>, Es...>::type;

    // Use this when transplanting an environment to a target that is an independent dynamic instance
    // as the existance of a part acts as a stamp of its respective acquisition to its environment.
    template<class = void>
    using RemoveDynamic = detail::RemoveDynamic<void(), void(Es...)>::type;
};

namespace detail {
    template<class Env>
    struct InsertMissing<Env>
    {
        using type = Env;
    };

    template<class Env, class E, class... Es>
    requires (Env::template HasTag<typename E::Tag>)
    struct InsertMissing<Env, E, Es...>
    {
        // Drop E as Env already has the part
        using type = InsertMissing<Env, Es...>::type;
    };

    template<class... Es, class E, class... Rest>
    struct InsertMissing<Environment<Es...>, E, Rest...>
    {
        using type = InsertMissing<Environment<Es..., E>, Rest...>::type;
    };

    template<class Keep, class E, class... Es>
    requires (E::isDynamic())
    struct RemoveDynamic<Keep, void(E, Es...)>
    {
        using type = RemoveDynamic<Keep, void(Es...)>::type;
    };

    template<class... Keep, class E, class... Es>
    struct RemoveDynamic<void(Keep...), void(E, Es...)>
    {
        using type = RemoveDynamic<void(Keep..., E), void(Es...)>::type;
    };

    template<class... Es>
    struct RemoveDynamic<void(Es...), void()>
    {
        using type = Environment<Es...>;
    };

    template<class Env, class... Es>
    consteval bool lhasAllTagsOfRImpl(Environment<Es...>)
    {
        return (... and Env::template HasTag<typename Es::Tag>);
    }

    template<class EnvL, class EnvR>
    inline constexpr bool lHasAllTagsOfR = lhasAllTagsOfRImpl<EnvL>(EnvR{});

    template<class Env, class... Es>
    auto insertMissingImpl(Environment<Es...>) -> Env::template InsertMissing<Es...>;

    template<class EnvL, class EnvR>
    using InsertMissingPartsOfRtoL = decltype(insertMissingImpl<EnvL>(EnvR{}));
}

ARC_MODULE_EXPORT
template<EnvironmentPart... Es, class Target>
constexpr auto& mergeEnvParts(Target& t)
{
    static_assert(sizeof...(Es) > 0);
    using NewEnv = Target::Environment::template Merge<Es...>;
    if constexpr (std::is_same_v<NewEnv, typename Target::Environment>)
        return t;
    else
        return detail::downCast<EnvironmentOverlay<NewEnv, std::remove_const_t<Target>>>(t);
}

ARC_MODULE_EXPORT
template<class Environment, class Target>
requires (not EnvironmentPart<Environment>)
constexpr auto& withEnv(Target& t)
{
    static_assert(Target::Environment::IsEmpty);
    if constexpr (Environment::IsEmpty)
        return t;
    else
        return detail::downCast<EnvironmentOverlay<Environment, std::remove_const_t<Target>>>(t);
}

namespace detail {
    template<class Env, class Target>
    requires (Env::IsEmpty) or (detail::lHasAllTagsOfR<typename Target::Environment, Env>)
    auto transferEnv() -> Target;
    template<class Env, class Target>
    auto transferEnv() -> EnvironmentOverlay<detail::InsertMissingPartsOfRtoL<typename Target::Environment, Env>, Target>;
}

// Any new tags in Env are added to Target's environment
// Existing tags in Target are not replaced
ARC_MODULE_EXPORT
template<class Env, class Target>
using TransferEnv = decltype(detail::transferEnv<Env, std::remove_const_t<Target>>());

}


#endif // INCLUDE_ARC_ENVIRONMENT_HPP
