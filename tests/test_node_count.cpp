import arc.tests.count;
import arc;

/*
arc-embed-begin

export module arc.tests.count;

namespace arc::tests::count {

cluster InnerUnary [R = Root]
{
    c = R::Node
}

cluster InnerBinary [R = Root]
{
    d = R::Node
    e = R::Node
}

cluster OuterCluster [R = Root]
{
    a = R::Node
    b = R::Node
    innerUnary = InnerUnary
    innerBinary = InnerBinary
}

}

arc-embed-end
*/

namespace arc::tests::count {

struct Node : arc::Node
{
    using Traits = arc::Traits<Node>;
};
struct Root
{
    using Node = count::Node;
};

using Graph = arc::Graph<OuterCluster, Root>;

static_assert(arc::IsUnary<decltype(Graph::a)>);
static_assert(arc::IsUnary<decltype(Graph::innerUnary)>);
static_assert(arc::IsUnary<decltype(Graph::innerUnary.c)>);
static_assert(arc::IsUnary<decltype(Graph::innerBinary.d)>);
static_assert(arc::IsUnary<decltype(Graph::innerBinary.e)>);
static_assert(not arc::IsUnary<decltype(Graph::innerBinary)>);
static_assert(not arc::IsUnary<Graph>);

}
