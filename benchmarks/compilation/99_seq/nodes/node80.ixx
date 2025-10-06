export module arc.bench.compile99_seq.node80;

import arc;
export import arc.bench.compile99_seq.trait.trait79;
export import arc.bench.compile99_seq.trait.trait80;

namespace arc::bench::compile99_seq {

export
struct Node80 : arc::Node
{
    using Depends = arc::Depends<trait::Trait79>;
    using Traits = arc::Traits<Node80, trait::Trait80>;

    int impl(this auto const& self, trait::Trait80::get)
    {
        return self.i + self.getNode(trait::trait79).get();
    }

    Node80() = default;
    int i = 80;
};

}
