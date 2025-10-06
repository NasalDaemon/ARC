export module arc.bench.compile99_seq.node31;

import arc;
export import arc.bench.compile99_seq.trait.trait30;
export import arc.bench.compile99_seq.trait.trait31;

namespace arc::bench::compile99_seq {

export
struct Node31 : arc::Node
{
    using Depends = arc::Depends<trait::Trait30>;
    using Traits = arc::Traits<Node31, trait::Trait31>;

    int impl(this auto const& self, trait::Trait31::get)
    {
        return self.i + self.getNode(trait::trait30).get();
    }

    Node31() = default;
    int i = 31;
};

}
