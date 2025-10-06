export module arc.bench.compile99_seq.node40;

import arc;
export import arc.bench.compile99_seq.trait.trait39;
export import arc.bench.compile99_seq.trait.trait40;

namespace arc::bench::compile99_seq {

export
struct Node40 : arc::Node
{
    using Depends = arc::Depends<trait::Trait39>;
    using Traits = arc::Traits<Node40, trait::Trait40>;

    int impl(this auto const& self, trait::Trait40::get)
    {
        return self.i + self.getNode(trait::trait39).get();
    }

    Node40() = default;
    int i = 40;
};

}
