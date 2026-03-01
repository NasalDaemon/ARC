export module arc.bench.compile99_seq.node52;

import arc;
export import arc.bench.compile99_seq.trait.trait51;
export import arc.bench.compile99_seq.trait.trait52;

namespace arc::bench::compile99_seq {

export
struct Node52 : arc::Node
{
    using Depends = arc::Depends<Trait51>;
    using Traits = arc::Traits<Trait52>;

    int impl(this auto const& self, Trait52::get)
    {
        return self.i + self.getNode(trait51).get();
    }

    Node52() = default;
    int i = 52;
};

}
