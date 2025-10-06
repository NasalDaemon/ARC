export module arc.bench.compile99_seq.node21;

import arc;
export import arc.bench.compile99_seq.trait.trait20;
export import arc.bench.compile99_seq.trait.trait21;

namespace arc::bench::compile99_seq {

export
struct Node21 : arc::Node
{
    using Depends = arc::Depends<trait::Trait20>;
    using Traits = arc::Traits<Node21, trait::Trait21>;

    int impl(this auto const& self, trait::Trait21::get)
    {
        return self.i + self.getNode(trait::trait20).get();
    }

    Node21() = default;
    int i = 21;
};

}
