export module arc.bench.compile99_seq.node32;

import arc;
export import arc.bench.compile99_seq.trait.trait31;
export import arc.bench.compile99_seq.trait.trait32;

namespace arc::bench::compile99_seq {

export
struct Node32 : arc::Node
{
    using Depends = arc::Depends<trait::Trait31>;
    using Traits = arc::Traits<Node32, trait::Trait32>;

    int impl(this auto const& self, trait::Trait32::get)
    {
        return self.i + self.getNode(trait::trait31).get();
    }

    Node32() = default;
    int i = 32;
};

}
