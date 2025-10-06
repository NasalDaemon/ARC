export module arc.bench.compile99_seq.node7;

import arc;
export import arc.bench.compile99_seq.trait.trait6;
export import arc.bench.compile99_seq.trait.trait7;

namespace arc::bench::compile99_seq {

export
struct Node7 : arc::Node
{
    using Depends = arc::Depends<trait::Trait6>;
    using Traits = arc::Traits<Node7, trait::Trait7>;

    int impl(this auto const& self, trait::Trait7::get)
    {
        return self.i + self.getNode(trait::trait6).get();
    }

    Node7() = default;
    int i = 7;
};

}
