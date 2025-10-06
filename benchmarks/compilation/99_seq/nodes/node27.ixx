export module arc.bench.compile99_seq.node27;

import arc;
export import arc.bench.compile99_seq.trait.trait26;
export import arc.bench.compile99_seq.trait.trait27;

namespace arc::bench::compile99_seq {

export
struct Node27 : arc::Node
{
    using Depends = arc::Depends<trait::Trait26>;
    using Traits = arc::Traits<Node27, trait::Trait27>;

    int impl(this auto const& self, trait::Trait27::get)
    {
        return self.i + self.getNode(trait::trait26).get();
    }

    Node27() = default;
    int i = 27;
};

}
