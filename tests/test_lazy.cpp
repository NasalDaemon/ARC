#include "arc/macros.hpp"

#if !ARC_IMPORT_STD
#include <any>
#include <span>
#include <variant>
#include <vector>
#endif

import arc;

#include "arc/doctest.h"

namespace arc::tests::lazy {

struct Trait : arc::UnconstrainedTrait
{
    #define ARC_METHODS_Trait(TAG) \
        TAG(get)

    ARC_METHODS(Trait)
} inline constexpr trait{};

struct Node : arc::Node
{
    using Traits = arc::Traits<Trait>;

    template<class Self>
    int impl(this Self& self, Trait::get)
    {
        static_assert(std::is_same_v<arc::InnerNodeHandle<arc::ContextOf<Self>>, Node>);
        return self.i + int(self.getNode(trait).get());
    }

    Node(int i = 88) : i(i) {}

    int i;
};

TEST_CASE("std::variant element offset")
{
    std::variant<int, double> v{9};
    CHECK(static_cast<void*>(&v) == static_cast<void*>(&std::get<int>(v)));
}

TEST_CASE("arc::Lazy")
{
    arc::test::Graph<arc::Lazy<Node>> g{.node{33}};
    g.mocks->methodReturns<Trait::get>(10);
    auto g2 = std::move(g);
    auto g3 = g2;

    CHECK(g3.node->i == 33);
    CHECK(g3.node.asTrait(trait).get() == 43);

    using LazyCtx = arc::ContextOf<std::remove_cvref_t<decltype(g3.node)>>;
    static_assert(std::is_same_v<arc::InnerNodeHandle<LazyCtx>, arc::Lazy<Node>>);
}

} // namespace arc::tests::lazy
