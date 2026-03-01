export module arc.bench.compile99_seq.node75;

import arc;
export import arc.bench.compile99_seq.trait.trait74;
export import arc.bench.compile99_seq.trait.trait75;

namespace arc::bench::compile99_seq {

export
struct Node75 : arc::Node
{
    using Depends = arc::Depends<Trait74>;
    using Traits = arc::Traits<Trait75>;

    int impl(this auto const& self, Trait75::get)
    {
        return self.i + self.getNode(trait74).get();
    }

    Node75() = default;
    int i = 75;
};

}
