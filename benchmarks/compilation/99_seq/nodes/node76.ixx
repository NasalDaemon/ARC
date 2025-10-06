export module arc.bench.compile99_seq.node76;

import arc;
export import arc.bench.compile99_seq.trait.trait75;
export import arc.bench.compile99_seq.trait.trait76;

namespace arc::bench::compile99_seq {

export
struct Node76 : arc::Node
{
    using Depends = arc::Depends<trait::Trait75>;
    using Traits = arc::Traits<Node76, trait::Trait76>;

    int impl(this auto const& self, trait::Trait76::get)
    {
        return self.i + self.getNode(trait::trait75).get();
    }

    Node76() = default;
    int i = 76;
};

}
