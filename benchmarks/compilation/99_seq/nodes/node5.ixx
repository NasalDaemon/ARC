export module arc.bench.compile99_seq.node5;

import arc;
export import arc.bench.compile99_seq.trait.trait4;
export import arc.bench.compile99_seq.trait.trait5;

namespace arc::bench::compile99_seq {

export
struct Node5 : arc::Node
{
    using Depends = arc::Depends<trait::Trait4>;
    using Traits = arc::Traits<Node5, trait::Trait5>;

    int impl(this auto const& self, trait::Trait5::get)
    {
        return self.i + self.getNode(trait::trait4).get();
    }

    Node5() = default;
    int i = 5;
};

}
