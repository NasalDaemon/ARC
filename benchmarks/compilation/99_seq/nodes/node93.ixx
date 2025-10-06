export module arc.bench.compile99_seq.node93;

import arc;
export import arc.bench.compile99_seq.trait.trait92;
export import arc.bench.compile99_seq.trait.trait93;

namespace arc::bench::compile99_seq {

export
struct Node93 : arc::Node
{
    using Depends = arc::Depends<trait::Trait92>;
    using Traits = arc::Traits<Node93, trait::Trait93>;

    int impl(this auto const& self, trait::Trait93::get)
    {
        return self.i + self.getNode(trait::trait92).get();
    }

    Node93() = default;
    int i = 93;
};

}
