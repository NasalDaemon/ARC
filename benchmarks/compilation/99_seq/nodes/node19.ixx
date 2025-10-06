export module arc.bench.compile99_seq.node19;

import arc;
export import arc.bench.compile99_seq.trait.trait18;
export import arc.bench.compile99_seq.trait.trait19;

namespace arc::bench::compile99_seq {

export
struct Node19 : arc::Node
{
    using Depends = arc::Depends<trait::Trait18>;
    using Traits = arc::Traits<Node19, trait::Trait19>;

    int impl(this auto const& self, trait::Trait19::get)
    {
        return self.i + self.getNode(trait::trait18).get();
    }

    Node19() = default;
    int i = 19;
};

}
