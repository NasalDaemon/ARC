#pragma once

#include "arc/detail/cast.hpp"
#include "arc/detail/compress.hpp"
#include "arc/function.hpp"
#include "arc/global_trait.hpp"
#include "arc/link.hpp"
#include "arc/macros.hpp"
#include "arc/node.hpp"
#include "arc/trait.hpp"

#if !ARC_IMPORT_STD
#include <utility>
#include <variant>
#endif

namespace arc {

ARC_MODULE_EXPORT
template<IsNodeHandle Underlying>
struct Lazy
{
    template<class Context>
    struct Node : arc::Node
    {
    private:
        ARC_COLD
        void initialise() const
        {
            auto* init = std::get_if<Initialiser>(&state);
            [[assume(init != nullptr)]];
            auto initKeepAlive = std::move(*init);
            initKeepAlive(this);
        }

        struct InnerContext : Context
        {
            template<IsTrait Trait>
            requires detail::HasLink<Context, Trait> or IsGlobalTrait<Trait>
            static constexpr auto getNode(auto& state, Trait trait)
            {
                // Assumption that variant-to-element offset=0 is tested in test_lazy.cpp
                auto const nodePtr = std::bit_cast<Node NodeState::*>(-ARC_MEM_PTR(Lazy::Node<Context>, state).toOffset());
                return Context{}.getNode(detail::downCast<NodeState>(state).*nodePtr, trait);
            }
        };

        using NodeState = ToNodeWrapper<Underlying>::template Node<detail::CompressContext<InnerContext>>;
        using Initialiser = Function<void(Node const*), FunctionPolicy{.copyable=true, .mutableCall=true, .constCall=false}>;
        using Variant = std::variant<NodeState, Initialiser>;
        Variant mutable state;

        template<class Trait>
        requires HasTrait<NodeState, Trait>
        using TraitsTemplate = arc::ResolvedTrait<Node, typename detail::ResolveTrait<NodeState, Trait>::template Types<>>;

    public:
        using Traits = arc::TraitsTemplate<Node, TraitsTemplate>;

        ARC_INLINE constexpr decltype(auto) impl(this auto& self, auto&&... args)
        {
            return self.getState().impl(ARC_FWD(args)...);
        }

        template<class Self>
        constexpr auto& getState(this Self& self)
        {
            if (not std::holds_alternative<NodeState>(self.state)) [[unlikely]]
                self.initialise();
            auto* node = std::get_if<NodeState>(&self.state);
            [[assume(node != nullptr)]];
            return std::forward_like<Self&>(*node);
        }

        constexpr explicit Node(auto&&... args)
            : state(Initialiser(
                [...args = ARC_FWD(args)](Node const* self) mutable -> void
                {
                    self->state.template emplace<NodeState>(std::move(args)...);
                }))
        {}
    };
};

}
