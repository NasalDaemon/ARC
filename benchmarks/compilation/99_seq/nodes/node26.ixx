export module arc.bench.compile99_seq.node26;

import arc;
export import arc.bench.compile99_seq.trait.trait25;
export import arc.bench.compile99_seq.trait.trait26;

namespace arc::bench::compile99_seq {

export
struct Node26 : arc::Node
{
    using Depends = arc::Depends<trait::Trait25>;
    using Traits = arc::Traits<Node26, trait::Trait26>;

    int impl(this auto const& self, trait::Trait26::get)
    {
        return self.i + self.getNode(trait::trait25).get();
    }

    Node26() = default;
    int i = 26;
};

}
