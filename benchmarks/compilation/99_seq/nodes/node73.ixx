export module arc.bench.compile99_seq.node73;

import arc;
export import arc.bench.compile99_seq.trait.trait72;
export import arc.bench.compile99_seq.trait.trait73;

namespace arc::bench::compile99_seq {

export
struct Node73 : arc::Node
{
    using Depends = arc::Depends<trait::Trait72>;
    using Traits = arc::Traits<Node73, trait::Trait73>;

    int impl(this auto const& self, trait::Trait73::get)
    {
        return self.i + self.getNode(trait::trait72).get();
    }

    Node73() = default;
    int i = 73;
};

}
