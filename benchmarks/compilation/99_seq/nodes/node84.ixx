export module arc.bench.compile99_seq.node84;

import arc;
export import arc.bench.compile99_seq.trait.trait83;
export import arc.bench.compile99_seq.trait.trait84;

namespace arc::bench::compile99_seq {

export
struct Node84 : arc::Node
{
    using Depends = arc::Depends<trait::Trait83>;
    using Traits = arc::Traits<Node84, trait::Trait84>;

    int impl(this auto const& self, trait::Trait84::get)
    {
        return self.i + self.getNode(trait::trait83).get();
    }

    Node84() = default;
    int i = 84;
};

}
