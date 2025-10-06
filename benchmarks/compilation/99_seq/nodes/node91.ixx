export module arc.bench.compile99_seq.node91;

import arc;
export import arc.bench.compile99_seq.trait.trait90;
export import arc.bench.compile99_seq.trait.trait91;

namespace arc::bench::compile99_seq {

export
struct Node91 : arc::Node
{
    using Depends = arc::Depends<trait::Trait90>;
    using Traits = arc::Traits<Node91, trait::Trait91>;

    int impl(this auto const& self, trait::Trait91::get)
    {
        return self.i + self.getNode(trait::trait90).get();
    }

    Node91() = default;
    int i = 91;
};

}
