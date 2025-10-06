export module arc.bench.compile99_seq.node9;

import arc;
export import arc.bench.compile99_seq.trait.trait8;
export import arc.bench.compile99_seq.trait.trait9;

namespace arc::bench::compile99_seq {

export
struct Node9 : arc::Node
{
    using Depends = arc::Depends<trait::Trait8>;
    using Traits = arc::Traits<Node9, trait::Trait9>;

    int impl(this auto const& self, trait::Trait9::get)
    {
        return self.i + self.getNode(trait::trait8).get();
    }

    Node9() = default;
    int i = 9;
};

}
