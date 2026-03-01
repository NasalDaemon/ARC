export module arc.bench.compile99_seq.node6;

import arc;
export import arc.bench.compile99_seq.trait.trait5;
export import arc.bench.compile99_seq.trait.trait6;

namespace arc::bench::compile99_seq {

export
struct Node6 : arc::Node
{
    using Depends = arc::Depends<Trait5>;
    using Traits = arc::Traits<Trait6>;

    int impl(this auto const& self, Trait6::get)
    {
        return self.i + self.getNode(trait5).get();
    }

    Node6() = default;
    int i = 6;
};

}
