#ifndef INCLUDE_ARC_FINALISE_HPP
#define INCLUDE_ARC_FINALISE_HPP

#include "arc/alias.hpp"
#include "arc/context_fwd.hpp"
#include "arc/detail/concepts.hpp"
#include "arc/environment.hpp"
#include "arc/macros.hpp"
#include "arc/key.hpp"

#if !ARC_IMPORT_STD
#include <tuple>
#endif

namespace arc {

namespace detail {

    template<class Source, class Target>
    consteval auto accessTagsRequiredFromKeys()
    {
        auto sourceRequires = ContextOf<Source>::Info::template requiresKeysToTarget<Source, Target>();
        auto targetRequires = ContextOf<Target>::Info::template requiresKeysToTarget<Source, Target>();
        return std::tuple_cat(sourceRequires, targetRequires);
    }

    template<class Source, class Target, arc::key::IsKey Key>
    consteval bool shouldAcquireAccess()
    {
        auto accessTags = accessTagsRequiredFromKeys<Source, Target>();
        if (std::tuple_size_v<decltype(accessTags)> > 0)
        {
            std::apply(
                []<class... Tags>(Tags...)
                {
                    Key::template assertCanAcquireAccess<Tags...>();
                },
                accessTags);
            return true;
        }
        return false;
    }

} // namespace detail

ARC_MODULE_EXPORT
template<bool ConsumeKey = true, class Source, class Target>
ARC_INLINE constexpr auto finalise(Source&, Target& target)
{
    constexpr auto accessTags = detail::accessTagsRequiredFromKeys<Source, Target>();
    if constexpr (std::tuple_size_v<decltype(accessTags)> != 0)
        static_assert(detail::alwaysFalse<Source, Target>, "Incorrect or insufficient keys to acquire access to target from source");
    return makeAlias(target);
}

// If ConsumeKey is false, the first key not needed to acquire access will be included in the alias
ARC_MODULE_EXPORT
template<bool ConsumeKey = true, class Source, class Target, class Key, class... Keys>
constexpr auto finalise(Source& source, Target& target, Key const& key, Keys const&... keys)
{
    if constexpr (detail::shouldAcquireAccess<Source, Target, Key>())
    {
        // Always consume the key to acquire access as we will lose the original source after finalising
        auto& target2 = key.acquireAccess(source, target);
        return finalise<ConsumeKey>(source, target2, keys...);
    }
    else
    {
        using WithEnv = arc::TransferEnv<typename Source::Environment, Target>;
        if constexpr (ConsumeKey)
        {
            using FinalInterface = Key::template Interface<WithEnv>;
            return makeAlias(detail::downCast<FinalInterface>(target), keys...);
        }
        else
        {
            return makeAlias(detail::downCast<WithEnv>(target), key, keys...);
        }
    }
}

} // namespace arc

#endif // INCLUDE_ARC_FINALISE_HPP
