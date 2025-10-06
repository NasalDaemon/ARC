export module arc.bench.compile99_seq.node53;

import arc;
export import arc.bench.compile99_seq.trait.trait52;
export import arc.bench.compile99_seq.trait.trait53;

namespace arc::bench::compile99_seq {

export
struct Node53 : arc::Node
{
    using Depends = arc::Depends<trait::Trait52>;
    using Traits = arc::Traits<Node53, trait::Trait53>;

    int impl(this auto const& self, trait::Trait53::get)
    {
        return self.i + self.getNode(trait::trait52).get();
    }

    Node53() = default;
    int i = 53;
};

}
