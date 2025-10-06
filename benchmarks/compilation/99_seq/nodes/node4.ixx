export module arc.bench.compile99_seq.node4;

import arc;
export import arc.bench.compile99_seq.trait.trait3;
export import arc.bench.compile99_seq.trait.trait4;

namespace arc::bench::compile99_seq {

export
struct Node4 : arc::Node
{
    using Depends = arc::Depends<trait::Trait3>;
    using Traits = arc::Traits<Node4, trait::Trait4>;

    int impl(this auto const& self, trait::Trait4::get)
    {
        return self.i + self.getNode(trait::trait3).get();
    }

    Node4() = default;
    int i = 4;
};

}
