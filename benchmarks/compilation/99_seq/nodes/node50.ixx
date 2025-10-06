export module arc.bench.compile99_seq.node50;

import arc;
export import arc.bench.compile99_seq.trait.trait49;
export import arc.bench.compile99_seq.trait.trait50;

namespace arc::bench::compile99_seq {

export
struct Node50 : arc::Node
{
    using Depends = arc::Depends<trait::Trait49>;
    using Traits = arc::Traits<Node50, trait::Trait50>;

    int impl(this auto const& self, trait::Trait50::get)
    {
        return self.i + self.getNode(trait::trait49).get();
    }

    Node50() = default;
    int i = 50;
};

}
