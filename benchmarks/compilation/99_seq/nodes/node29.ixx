export module arc.bench.compile99_seq.node29;

import arc;
export import arc.bench.compile99_seq.trait.trait28;
export import arc.bench.compile99_seq.trait.trait29;

namespace arc::bench::compile99_seq {

export
struct Node29 : arc::Node
{
    using Depends = arc::Depends<trait::Trait28>;
    using Traits = arc::Traits<Node29, trait::Trait29>;

    int impl(this auto const& self, trait::Trait29::get)
    {
        return self.i + self.getNode(trait::trait28).get();
    }

    Node29() = default;
    int i = 29;
};

}
