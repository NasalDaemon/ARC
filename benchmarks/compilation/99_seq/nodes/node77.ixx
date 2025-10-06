export module arc.bench.compile99_seq.node77;

import arc;
export import arc.bench.compile99_seq.trait.trait76;
export import arc.bench.compile99_seq.trait.trait77;

namespace arc::bench::compile99_seq {

export
struct Node77 : arc::Node
{
    using Depends = arc::Depends<trait::Trait76>;
    using Traits = arc::Traits<Node77, trait::Trait77>;

    int impl(this auto const& self, trait::Trait77::get)
    {
        return self.i + self.getNode(trait::trait76).get();
    }

    Node77() = default;
    int i = 77;
};

}
