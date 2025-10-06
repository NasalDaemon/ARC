export module arc.bench.compile99_seq.node52;

import arc;
export import arc.bench.compile99_seq.trait.trait51;
export import arc.bench.compile99_seq.trait.trait52;

namespace arc::bench::compile99_seq {

export
struct Node52 : arc::Node
{
    using Depends = arc::Depends<trait::Trait51>;
    using Traits = arc::Traits<Node52, trait::Trait52>;

    int impl(this auto const& self, trait::Trait52::get)
    {
        return self.i + self.getNode(trait::trait51).get();
    }

    Node52() = default;
    int i = 52;
};

}
