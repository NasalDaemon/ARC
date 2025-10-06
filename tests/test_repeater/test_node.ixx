export module arc.tests.repeater.test_node;

import arc.tests.repeater.traits;

import arc;

export namespace arc::tests::repeater {

struct TestNode : arc::Node
{
    void impl(trait::Trait::function, int& i) const
    {
        i++;
    }
    void impl(this auto const& self, trait::Trait::defer, int& i)
    {
        self.getNode(trait::trait).function(i);
    }

    using Traits = arc::Traits<TestNode, trait::Trait>;
};

} // namespace arc::tests::repeater
