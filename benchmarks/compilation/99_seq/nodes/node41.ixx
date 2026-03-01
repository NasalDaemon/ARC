export module arc.bench.compile99_seq.node41;

import arc;
export import arc.bench.compile99_seq.trait.trait40;
export import arc.bench.compile99_seq.trait.trait41;

namespace arc::bench::compile99_seq {

export
struct Node41 : arc::Node
{
    using Depends = arc::Depends<Trait40>;
    using Traits = arc::Traits<Trait41>;

    int impl(this auto const& self, Trait41::get)
    {
        return self.i + self.getNode(trait40).get();
    }

    Node41() = default;
    int i = 41;
};

}
