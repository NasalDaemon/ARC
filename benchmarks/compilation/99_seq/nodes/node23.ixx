export module arc.bench.compile99_seq.node23;

import arc;
export import arc.bench.compile99_seq.trait.trait22;
export import arc.bench.compile99_seq.trait.trait23;

namespace arc::bench::compile99_seq {

export
struct Node23 : arc::Node
{
    using Depends = arc::Depends<Trait22>;
    using Traits = arc::Traits<Trait23>;

    int impl(this auto const& self, Trait23::get)
    {
        return self.i + self.getNode(trait22).get();
    }

    Node23() = default;
    int i = 23;
};

}
