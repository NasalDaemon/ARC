export module arc.bench.compile99_seq.node61;

import arc;
export import arc.bench.compile99_seq.trait.trait60;
export import arc.bench.compile99_seq.trait.trait61;

namespace arc::bench::compile99_seq {

export
struct Node61 : arc::Node
{
    using Depends = arc::Depends<trait::Trait60>;
    using Traits = arc::Traits<Node61, trait::Trait61>;

    int impl(this auto const& self, trait::Trait61::get)
    {
        return self.i + self.getNode(trait::trait60).get();
    }

    Node61() = default;
    int i = 61;
};

}
