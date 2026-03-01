export module arc.bench.compile99_seq.node44;

import arc;
export import arc.bench.compile99_seq.trait.trait43;
export import arc.bench.compile99_seq.trait.trait44;

namespace arc::bench::compile99_seq {

export
struct Node44 : arc::Node
{
    using Depends = arc::Depends<Trait43>;
    using Traits = arc::Traits<Trait44>;

    int impl(this auto const& self, Trait44::get)
    {
        return self.i + self.getNode(trait43).get();
    }

    Node44() = default;
    int i = 44;
};

}
