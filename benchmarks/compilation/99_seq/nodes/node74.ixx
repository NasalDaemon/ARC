export module arc.bench.compile99_seq.node74;

import arc;
export import arc.bench.compile99_seq.trait.trait73;
export import arc.bench.compile99_seq.trait.trait74;

namespace arc::bench::compile99_seq {

export
struct Node74 : arc::Node
{
    using Depends = arc::Depends<trait::Trait73>;
    using Traits = arc::Traits<Node74, trait::Trait74>;

    int impl(this auto const& self, trait::Trait74::get)
    {
        return self.i + self.getNode(trait::trait73).get();
    }

    Node74() = default;
    int i = 74;
};

}
