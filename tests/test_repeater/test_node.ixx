module;
#include "arc/macros.hpp"

#if !ARC_IMPORT_STD
#include <cstddef>
#endif

export module arc.tests.repeater.test_node;

import arc.tests.repeater.traits;

import arc;

export namespace arc::tests::repeater {

struct SourceNode : arc::Node
{
    using Traits = arc::Traits<trait::Source>;

    void impl(this auto& self, trait::Source::defer, int& i)
    {
        self.getNode(trait::target).function(i);
    }
};

struct TargetNode : arc::Node
{
    using Traits = arc::Traits<trait::Target>;

    void impl(trait::Target::function, int& i)
    {
        i++;
        functionCalled++;
    }

    std::size_t functionCalled = 0;
};

struct TargetNode2 : TargetNode
{};

} // namespace arc::tests::repeater
