#pragma once

#include "arc/detail/cast.hpp"
#include "arc/detail/compress.hpp"
#include "arc/detail/storage.hpp"
#include "arc/cluster_fwd.hpp"
#include "arc/function.hpp"
#include "arc/global_trait.hpp"
#include "arc/link.hpp"
#include "arc/macros.hpp"
#include "arc/node.hpp"
#include "arc/trait.hpp"

#if !ARC_IMPORT_STD
#include <memory>
#include <utility>
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
            initialiser(this);
            initialiser.reset(); // only reset if initialiser did not throw
            state.value()->visit(detail::OnGraphConstructedVisitor{});
        }

        struct InnerContext : Context
        {
            template<IsTrait Trait>
            requires detail::HasLocalLink<Context, Trait> or IsGlobalTrait<Trait>
            static constexpr auto getNode(auto& state, Trait trait)
            {
                auto const nodePtr = detail::memberPtr<Node>(std::bit_cast<NodeState Node::*>(&Node::state));
                return Context{}.getNode(nodePtr.getClassFromMember(state), trait);
            }
        };

        using Initialiser = Function<void(Node const*), FunctionPolicy{.copyable=true, .mutableCall=true, .constCall=false}>;
        using NodeState = ToNodeWrapper<Underlying>::template Node<detail::CompressContext<InnerContext>>;
        Initialiser mutable initialiser;
        detail::Storage<NodeState> mutable state; // alive when initialiser is falsey

        template<class Trait>
        requires HasTrait<NodeState, Trait>
        using TraitsTemplate = arc::ResolvedTrait<Node, detail::ResolveTypesOfNode<NodeState, Trait>>;

    public:
        using Traits = arc::TraitsTemplate<TraitsTemplate>;

        ARC_INLINE constexpr decltype(auto) impl(this auto& self, auto&&... args)
        {
            return self.getState().impl(ARC_FWD(args)...);
        }

        ARC_INLINE constexpr bool hasState() const { return not initialiser; }

        template<class Self>
        constexpr detail::ConstLike<Self, NodeState>& getState(this Self& self)
        {
            if (not self.hasState()) [[unlikely]]
                self.initialise();
            return *self.state.value();
        }

        constexpr explicit Node(auto&&... args)
            : initialiser(
                [...args = ARC_FWD(args)](Node const* self) mutable -> void
                {
                    std::construct_at(self->state.storage(), std::move(args)...);
                })
        {}

        constexpr Node(Node const& other)
            : initialiser(other.initialiser)
        {
            if (hasState())
                std::construct_at(state.storage(), *other.state.value());
        }

        constexpr Node(Node&& other)
            : initialiser(std::move(other.initialiser))
        {
            if (hasState())
                std::construct_at(state.storage(), std::move(*other.state.value()));
            else // ensure that moved-from uninitialised Lazy nodes are left in a state that will throw if they are used
                other.initialiser = [](Node const*) -> void { throw std::runtime_error("arc::node::Lazy node moved from before initialisation"); };
        }

        constexpr ~Node()
        {
            if (hasState())
                std::destroy_at(state.value());
        }
    };
};

namespace node {
    ARC_MODULE_EXPORT
    using arc::Lazy;
}

}
