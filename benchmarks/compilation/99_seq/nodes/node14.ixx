export module arc.bench.compile99_seq.node14;

import arc;
export import arc.bench.compile99_seq.trait.trait13;
export import arc.bench.compile99_seq.trait.trait14;

namespace arc::bench::compile99_seq {

export
struct Node14 : arc::Node
{
    using Depends = arc::Depends<trait::Trait13>;
    using Traits = arc::Traits<Node14, trait::Trait14>;

    int impl(this auto const& self, trait::Trait14::get)
    {
        return self.i + self.getNode(trait::trait13).get();
    }

    Node14() = default;
    int i = 14;
};

}
