export module arc.bench.compile99_seq.node53;

import arc;
export import arc.bench.compile99_seq.trait.trait52;
export import arc.bench.compile99_seq.trait.trait53;

namespace arc::bench::compile99_seq {

export
struct Node53 : arc::Node
{
    using Depends = arc::Depends<Trait52>;
    using Traits = arc::Traits<Trait53>;

    int impl(this auto const& self, Trait53::get)
    {
        return self.i + self.getNode(trait52).get();
    }

    Node53() = default;
    int i = 53;
};

}
