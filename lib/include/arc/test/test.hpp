#ifndef INCLUDE_ARC_TEST_HPP
#define INCLUDE_ARC_TEST_HPP

#include "arc/assert_handlers.hpp"
#include "arc/cluster.hpp"
#include "arc/context.hpp"
#include "arc/depends.hpp"
#include "arc/global_graph.hpp"
#include "arc/graph.hpp"
#include "arc/key.hpp"
#include "arc/link.hpp"
#include "arc/macros.hpp"
#include "arc/nodes/map_info.hpp"
#include "arc/test/mock_fwd.hpp"
#include "arc/node.hpp"
#include "arc/node_with_fwd.hpp"
#include "arc/test_context.hpp"
#include "arc/trait.hpp"

#if !ARC_IMPORT_STD
#include <concepts>
#include <cstddef>
#endif

namespace arc::test {

ARC_MODULE_EXPORT
template<IsTrait Trait>
struct Local : Trait
{
    static TraitExpects<Trait> expects();
};

ARC_MODULE_EXPORT
template<IsTrait Trait>
constexpr Local<Trait> local(Trait = {}) { return {}; }

ARC_MODULE_EXPORT
template<IsTrait Trait>
struct MockTrait : Trait
{
    static TraitExpects<Trait> expects();

    // Mocks are to bypass checks, and may implement only what is needed for testing
    template<class Self, class...>
    using Implements = void;
};

ARC_MODULE_EXPORT
struct MockKey : arc::key::Default
{
    template<class T>
    using Trait = MockTrait<T>;
};

ARC_MODULE_EXPORT
struct TestOnlyNode : arc::Node
{
    template<class... Traits>
    using Impl = arc::Build<TestOnlyNode>::template Impl<Traits...>;

    template<arc::detail::IsDependsItem... DependTraits>
    using Uses = arc::Build<TestOnlyNode>::template Uses<DependTraits...>;

    template<class Self>
    static constexpr void assertNodeContext()
    {
        static_assert(IsTestContext<ContextOf<Self>>, "This node may only be used in a test context.");
        arc::Node::assertNodeContext<Self>();
    }
};

ARC_MODULE_EXPORT
template<arc::detail::IsDependsItem... Traits>
requires (sizeof...(Traits) > 0)
using TestOnlyNodeUses = Uses<TestOnlyNode, Traits...>;

ARC_MODULE_EXPORT
template<class... Traits>
requires (sizeof...(Traits) > 0)
using TestOnlyNodeImpl = Impl<TestOnlyNode, Traits...>;

namespace detail {
    template<class Root>
    auto getRootAssertHandler() -> arc::ThrowAssertHandler;
    template<class Root>
    requires requires { typename Root::ArcContractAssertHandler; }
    auto getRootAssertHandler() -> Root::ArcContractAssertHandler;

    struct TestMapInfo
    {
        template<IsContext Context>
        struct MapInfo : Context::Info
        {
            static void isTestContext(detail::TestContextTag);
        };
    };

    template<IsNodeHandle NodeT, IsNodeHandle MocksT>
    struct Cluster
    {
        template<class Context>
        struct Impl : arc::Cluster
        {
            struct Node;
            struct Mocks;
            using AssertHandler = decltype(getRootAssertHandler<typename Context::Root>());

            struct Node : arc::Context<Impl, NodeT>
            {
                ARC_LINK_TO_GLOBAL()

                static constexpr std::size_t Depth = Context::Depth;

                // Resolve to parent by default
                template<class Trait>
                requires arc::detail::HasLink<Context, Trait>
                static ResolvedLink<Context, Trait> resolveLink(Trait, arc::LinkExact<Trait>);

                // Otherwise resolve global to mocks
                template<class Trait>
                static ResolvedLink<Mocks, Trait> resolveLink(arc::Global<Trait>, arc::LinkPriorityMax);

                // Otherwise resolve to mocks
                template<class Trait>
                static ResolvedLink<Mocks, Trait> resolveLink(Trait, arc::LinkPriorityMin);

                // getNode calls to mocks, so allow partial implementation of traits
                struct Info : Context::Info
                {
                    using DefaultKey = MockKey;
                    static constexpr AssertHandler ContractAssert{};
                };
            };

            struct Mocks : arc::Context<Impl, MocksT>
            {
                ARC_LINK_TO_GLOBAL()

                static constexpr std::size_t Depth = Context::Depth;

                // Allow explicitly resolving the node being tested
                template<class Trait>
                static ResolvedLink<Node, Trait> resolveLink(Local<Trait>, arc::LinkExact<Local<Trait>>);

                // Resolve to parent by default
                template<class Trait>
                requires arc::detail::HasLink<Context, Trait>
                static ResolvedLink<Context, Trait> resolveLink(Trait, arc::LinkPriorityMax);

                // Otherwise resolve to node being tested
                template<class Trait>
                static ResolvedLink<Node, Trait> resolveLink(Trait, arc::LinkPriorityMin);

                struct Info : Context::Info
                {
                    static constexpr AssertHandler ContractAssert{};
                };
            };

            template<class Trait>
            requires ContextHasTrait<Node, Trait>
            static ResolvedLink<Node, Trait> resolveLink(Trait, arc::LinkPriorityMax);
            template<class Trait>
            requires ContextHasTrait<Mocks, Trait>
            static ResolvedLink<Mocks, Trait> resolveLink(Trait, arc::LinkPriorityMin);

            ARC_NODE(Node, node)
            ARC_NODE(Mocks, mocks)

            constexpr auto* operator->(this auto& self) { return std::addressof(self.node); }

            constexpr void visit(this auto& self, auto&& visitor)
            {
                self.node.visit(visitor);
                self.mocks.visit(visitor);
            }
        };

        template<class Context>
        using Node = Impl<Context>;
    };

} // namespace detail

ARC_MODULE_EXPORT
template<IsNodeHandle Node, IsNodeHandle Mocks = Mock<>>
using Cluster = MapInfo<detail::Cluster<Node, Mocks>, detail::TestMapInfo>;

ARC_MODULE_EXPORT
template<IsNodeHandle Node, IsNodeHandle Mocks = Mock<>, class Root = void>
using Graph = arc::Graph<Cluster<Node, Mocks>, Root>;

ARC_MODULE_EXPORT
template<IsNodeHandle Node, class GlobalNode, IsNodeHandle Mocks = Mock<>, class Root = void>
using GraphWithGlobal = arc::GraphWithGlobal<Cluster<Node, Mocks>, GlobalNode, Root>;

} // namespace arc::test

namespace arc::detail {
    template<>
    inline constexpr bool isNodeBase<arc::test::TestOnlyNode> = true;
}

#endif // INCLUDE_ARC_TEST_HPP
