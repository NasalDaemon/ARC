export module arc.bench.compile99_seq.node30;

import arc;
export import arc.bench.compile99_seq.trait.trait29;
export import arc.bench.compile99_seq.trait.trait30;

namespace arc::bench::compile99_seq {

export
struct Node30 : arc::Node
{
    using Depends = arc::Depends<Trait29>;
    using Traits = arc::Traits<Trait30>;

    int impl(this auto const& self, Trait30::get)
    {
        return self.i + self.getNode(trait29).get();
    }

    Node30() = default;
    int i = 30;
};

}
