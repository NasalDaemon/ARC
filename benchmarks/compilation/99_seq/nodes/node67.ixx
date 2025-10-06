export module arc.bench.compile99_seq.node67;

import arc;
export import arc.bench.compile99_seq.trait.trait66;
export import arc.bench.compile99_seq.trait.trait67;

namespace arc::bench::compile99_seq {

export
struct Node67 : arc::Node
{
    using Depends = arc::Depends<trait::Trait66>;
    using Traits = arc::Traits<Node67, trait::Trait67>;

    int impl(this auto const& self, trait::Trait67::get)
    {
        return self.i + self.getNode(trait::trait66).get();
    }

    Node67() = default;
    int i = 67;
};

}
