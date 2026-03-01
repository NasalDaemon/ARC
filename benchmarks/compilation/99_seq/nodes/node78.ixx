export module arc.bench.compile99_seq.node78;

import arc;
export import arc.bench.compile99_seq.trait.trait77;
export import arc.bench.compile99_seq.trait.trait78;

namespace arc::bench::compile99_seq {

export
struct Node78 : arc::Node
{
    using Depends = arc::Depends<Trait77>;
    using Traits = arc::Traits<Trait78>;

    int impl(this auto const& self, Trait78::get)
    {
        return self.i + self.getNode(trait77).get();
    }

    Node78() = default;
    int i = 78;
};

}
