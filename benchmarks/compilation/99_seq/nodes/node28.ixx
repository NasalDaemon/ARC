export module arc.bench.compile99_seq.node28;

import arc;
export import arc.bench.compile99_seq.trait.trait27;
export import arc.bench.compile99_seq.trait.trait28;

namespace arc::bench::compile99_seq {

export
struct Node28 : arc::Node
{
    using Depends = arc::Depends<Trait27>;
    using Traits = arc::Traits<Trait28>;

    int impl(this auto const& self, Trait28::get)
    {
        return self.i + self.getNode(trait27).get();
    }

    Node28() = default;
    int i = 28;
};

}
