export module arc.bench.compile99_seq.node49;

import arc;
export import arc.bench.compile99_seq.trait.trait48;
export import arc.bench.compile99_seq.trait.trait49;

namespace arc::bench::compile99_seq {

export
struct Node49 : arc::Node
{
    using Depends = arc::Depends<trait::Trait48>;
    using Traits = arc::Traits<Node49, trait::Trait49>;

    int impl(this auto const& self, trait::Trait49::get)
    {
        return self.i + self.getNode(trait::trait48).get();
    }

    Node49() = default;
    int i = 49;
};

}
