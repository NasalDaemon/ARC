export module arc.bench.compile99_seq.node18;

import arc;
export import arc.bench.compile99_seq.trait.trait17;
export import arc.bench.compile99_seq.trait.trait18;

namespace arc::bench::compile99_seq {

export
struct Node18 : arc::Node
{
    using Depends = arc::Depends<trait::Trait17>;
    using Traits = arc::Traits<Node18, trait::Trait18>;

    int impl(this auto const& self, trait::Trait18::get)
    {
        return self.i + self.getNode(trait::trait17).get();
    }

    Node18() = default;
    int i = 18;
};

}
