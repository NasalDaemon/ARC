#ifndef INCLUDE_ARC_TRAIT_VIEW_HPP
#define INCLUDE_ARC_TRAIT_VIEW_HPP

#include "arc/trait_view_fwd.hpp"

#include "arc/empty_types.hpp"
#include "arc/global_context.hpp"
#include "arc/global_trait.hpp"
#include "arc/key.hpp"
#include "arc/macros.hpp"
#include "arc/no_trait.hpp"
#include "arc/trait.hpp"
#include "arc/traits/spy.hpp"

#if !ARC_IMPORT_STD
#include <functional>
#include <type_traits>
#endif

namespace arc {

namespace detail {
    struct Unknown
    {
        template<class T>
        explicit(false) constexpr operator T&&();
        template<class T>
        explicit(false) constexpr operator T&();
    };
}

struct AutoCompleteTraitNodeView
{
    ARC_INLINE constexpr auto* operator->(this auto&& self) { return std::addressof(self); }

    constexpr detail::Unknown visit(this auto&& self, auto&& visitor);
    constexpr detail::Unknown getElementId() const;
};

ARC_MODULE_EXPORT
template<IsTrait Trait>
struct AutoCompleteTraitView final : Trait::Meta::Methods
{
    static consteval std::true_type isTrait(MatchesTrait<Trait> auto) { return {}; }
    static consteval std::false_type isTrait(auto) { return {}; }

    using Types = EmptyTypes;
    struct Traits
    {
        template<IsTrait T>
        using ResolveInterface = AutoCompleteTraitView;

        template<IsTrait T>
        using ResolveTypes = Types;
    };

    template<IsMethodOf<Trait> Method>
    constexpr detail::Unknown impl(this auto&&, Method, auto&&...);

    constexpr AutoCompleteTraitNodeView operator->() const { return {}; }
};

template<IsTrait Trait, class ImplAlias>
struct TraitNodeView final
{
    constexpr explicit TraitNodeView(Trait, ImplAlias alias) : alias(alias) {}

    ARC_INLINE constexpr auto* operator->(this auto&& self) { return std::addressof(self); }

    // Visit the TraitView of the concrete implementation (e.g. active node of union)
    template<class Visitor>
    constexpr decltype(auto) visit(this auto&& self, Visitor&& visitor)
    {
        return self.alias->visit(
            [&](auto& impl) -> decltype(auto)
            {
                return std::invoke(ARC_FWD(Visitor, visitor), impl.asTrait(Trait{}));
            });
    }

    ARC_INLINE constexpr auto const& getElementId() const
    {
        return alias->getElementId();
    }

    ARC_INLINE constexpr auto getElementHandle() const
    {
        return alias->getElementHandle();
    }

private:
    ImplAlias alias;
};

// Presents a view over a trait implementation, where only the trait trait functions are accessible
ARC_MODULE_EXPORT
template<IsTrait Trait, class ImplAlias, class Types_ /*= EmptyTypes*/>
struct TraitView final : Trait::Meta::Methods
{
    constexpr TraitView(Trait, ImplAlias alias, std::type_identity<Types_>)
        : alias(alias)
    {
        static_assert(not IsTrait<TraitView>);
        ARC_ASSERT_IMPLEMENTS(typename ImplAlias::Interface, Types, Trait);
        if constexpr (not std::is_const_v<typename ImplAlias::Impl>)
            ARC_ASSERT_IMPLEMENTS(TraitView, Types, Trait);
    }

    static consteval std::true_type isTrait(MatchesTrait<Trait> auto) { return {}; }
    static consteval std::false_type isTrait(auto) { return {}; }

    using Types = detail::Decompress<Types_>;
    struct Traits
    {
        using Node = ImplAlias::Impl;

        template<std::same_as<TraitView> = TraitView>
        using GetContext = ContextOf<Node>;

        template<std::same_as<Trait> T>
        using ResolveInterface = TraitView;

        template<std::same_as<Trait> T>
        using ResolveTypes = Types;
    };

    template<IsMethodOf<Trait> Method>
    ARC_INLINE constexpr decltype(auto) impl(this auto&& self, Method method, auto&&... args)
    {
        return invoke(self.alias.get(), method, ARC_FWD(args)...);
    }

    template<class Self, IsMethodOf<Trait> Method>
    ARC_INLINE constexpr auto impl(this Self&& self, Method, arc::AsFunctor)
    {
        return MethodFunctor<Method>(self.alias);
    }

    ARC_INLINE auto operator->(this auto&& self)
    {
        return TraitNodeView(Trait{}, self.alias);
    }

private:
    template<class Method>
    struct MethodFunctor
    {
        ImplAlias alias;

        ARC_INLINE constexpr decltype(auto) operator()(this auto&& self, auto&&... args)
        {
            return invoke(self.alias.get(), Method{}, ARC_FWD(args)...);
        }
    };

    template<class Method, class... Args>
    ARC_INLINE static constexpr decltype(auto) invoke(auto& node, Method method, Args&&... args)
    {
        using Context = Traits::template GetContext<>;
        if constexpr (ContextHasGlobalTrait<Context, Global<trait::SpyOnly<Trait>>>)
        {
            auto const caller = [&node](auto&&... spyArgs) -> decltype(auto)
            {
                return node.impl(Method{}, static_cast<Args&&>(spyArgs)...);
            };

            return node.getGlobal(trait::spyOnly<Trait>).intercept(method, caller, ARC_FWD(args)...);
        }
        else
        {
            return node.impl(method, ARC_FWD(args)...);
        }
    }

    ImplAlias alias;
};

template<IsNoTrait Trait, class ImplAlias, class Types_>
struct TraitView<Trait, ImplAlias, Types_> final : Trait::Meta::Methods
{
    constexpr TraitView(Trait, ImplAlias alias, std::type_identity<Types_>)
        : alias(alias)
    {}

    static consteval std::false_type isTrait(auto) { return {}; }

    using Types = detail::Decompress<Types_>;
    struct Traits
    {
        using Node = ImplAlias::Impl;

        template<std::same_as<TraitView> = TraitView>
        using GetContext = ContextOf<typename ImplAlias::Impl>;

        template<std::same_as<Trait> T>
        using ResolveInterface = TraitView;

        template<std::same_as<Trait> T>
        using ResolveTypes = Types;
    };

    ARC_INLINE auto operator->(this auto&& self)
    {
        return self.alias;
    }

private:
    ImplAlias alias;
};

ARC_MODULE_EXPORT
template<IsTrait Trait, key::IsKey Key, key::IsKey... Keys>
constexpr IsTraitViewOf<Trait, Key> auto makeTraitView(auto& source, auto target, Trait, Key const& key, Keys const&... keys)
{
    return TraitView(key::Trait<Key, Trait>{}, target.ptr->finalise(source, key, keys...), target.types());
}

} // namespace arc


#endif // INCLUDE_ARC_TRAIT_VIEW_HPP
