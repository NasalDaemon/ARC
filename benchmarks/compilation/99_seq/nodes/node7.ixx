export module arc.bench.compile99_seq.node7;

import arc;
export import arc.bench.compile99_seq.trait.trait6;
export import arc.bench.compile99_seq.trait.trait7;

namespace arc::bench::compile99_seq {

export
struct Node7 : arc::Node
{
    using Depends = arc::Depends<Trait6>;
    using Traits = arc::Traits<Trait7>;

    int impl(this auto const& self, Trait7::get)
    {
        return self.i + self.getNode(trait6).get();
    }

    Node7() = default;
    int i = 7;
};

}
