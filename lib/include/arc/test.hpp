#ifndef INCLUDE_ARC_TEST_HPP
#define INCLUDE_ARC_TEST_HPP

#include "arc/cluster.hpp"
#include "arc/context.hpp"
#include "arc/global_graph.hpp"
#include "arc/graph.hpp"
#include "arc/key.hpp"
#include "arc/link.hpp"
#include "arc/macros.hpp"
#include "arc/map_info.hpp"
#include "arc/mock_fwd.hpp"
#include "arc/node.hpp"
#include "arc/test_context.hpp"
#include "arc/trait.hpp"

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
    template<class Self>
    static constexpr void assertNodeContext()
    {
        static_assert(IsTestContext<ContextOf<Self>>, "This node may only be used in a test context.");
        arc::Node::assertNodeContext<Self>();
    }
};

namespace detail {

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

            template<class Trait>
            static ResolvedLink<Node, Trait> resolveLink(Trait, arc::LinkPriorityMin);

            struct Node : arc::Context<Impl, NodeT>
            {
                static constexpr std::size_t Depth = Context::Depth;

                // Resolve to parent by default
                template<class Trait>
                requires arc::detail::HasLink<Context, Trait>
                static ResolvedLink<Context, Trait> resolveLink(Trait, arc::LinkPriorityMax);

                // Otherwise resolve to mocks
                template<class Trait>
                static ResolvedLink<Mocks, Trait> resolveLink(Trait, arc::LinkPriorityMin);

                // getNode calls to mocks, so allow partial implementation of traits
                struct Info : Context::Info
                {
                    using DefaultKey = MockKey;
                };
            };

            struct Mocks : arc::Context<Impl, MocksT>
            {
                static constexpr std::size_t Depth = Context::Depth;

                // Resolve to parent by default
                template<class Trait>
                requires arc::detail::HasLink<Context, Trait>
                static ResolvedLink<Context, Trait> resolveLink(Trait, arc::LinkPriorityMax);

                // Otherwise resolve to node being tested
                template<class Trait>
                static ResolvedLink<Node, Trait> resolveLink(Trait, arc::LinkPriorityMin);

                // Allow explicitly resolving the node being tested
                template<class Trait>
                static ResolvedLink<Node, Trait> resolveLink(Local<Trait>, arc::LinkExact<Local<Trait>>);
            };

            ARC_NODE(Node, node)
            ARC_NODE(Mocks, mocks)

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


#endif // INCLUDE_ARC_TEST_HPP
