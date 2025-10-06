export module arc.bench.compile99_seq.node13;

import arc;
export import arc.bench.compile99_seq.trait.trait12;
export import arc.bench.compile99_seq.trait.trait13;

namespace arc::bench::compile99_seq {

export
struct Node13 : arc::Node
{
    using Depends = arc::Depends<trait::Trait12>;
    using Traits = arc::Traits<Node13, trait::Trait13>;

    int impl(this auto const& self, trait::Trait13::get)
    {
        return self.i + self.getNode(trait::trait12).get();
    }

    Node13() = default;
    int i = 13;
};

}
