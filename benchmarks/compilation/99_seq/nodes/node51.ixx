export module arc.bench.compile99_seq.node51;

import arc;
export import arc.bench.compile99_seq.trait.trait50;
export import arc.bench.compile99_seq.trait.trait51;

namespace arc::bench::compile99_seq {

export
struct Node51 : arc::Node
{
    using Depends = arc::Depends<trait::Trait50>;
    using Traits = arc::Traits<Node51, trait::Trait51>;

    int impl(this auto const& self, trait::Trait51::get)
    {
        return self.i + self.getNode(trait::trait50).get();
    }

    Node51() = default;
    int i = 51;
};

}
