export module arc.bench.compile99_seq.node95;

import arc;
export import arc.bench.compile99_seq.trait.trait94;
export import arc.bench.compile99_seq.trait.trait95;

namespace arc::bench::compile99_seq {

export
struct Node95 : arc::Node
{
    using Depends = arc::Depends<trait::Trait94>;
    using Traits = arc::Traits<Node95, trait::Trait95>;

    int impl(this auto const& self, trait::Trait95::get)
    {
        return self.i + self.getNode(trait::trait94).get();
    }

    Node95() = default;
    int i = 95;
};

}
