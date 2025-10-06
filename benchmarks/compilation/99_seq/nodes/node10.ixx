export module arc.bench.compile99_seq.node10;

import arc;
export import arc.bench.compile99_seq.trait.trait9;
export import arc.bench.compile99_seq.trait.trait10;

namespace arc::bench::compile99_seq {

export
struct Node10 : arc::Node
{
    using Depends = arc::Depends<trait::Trait9>;
    using Traits = arc::Traits<Node10, trait::Trait10>;

    int impl(this auto const& self, trait::Trait10::get)
    {
        return self.i + self.getNode(trait::trait9).get();
    }

    Node10() = default;
    int i = 10;
};

}
