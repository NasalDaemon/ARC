export module arc.bench.compile99_seq.node51;

import arc;
export import arc.bench.compile99_seq.trait.trait50;
export import arc.bench.compile99_seq.trait.trait51;

namespace arc::bench::compile99_seq {

export
struct Node51 : arc::Node
{
    using Depends = arc::Depends<Trait50>;
    using Traits = arc::Traits<Trait51>;

    int impl(this auto const& self, Trait51::get)
    {
        return self.i + self.getNode(trait50).get();
    }

    Node51() = default;
    int i = 51;
};

}
