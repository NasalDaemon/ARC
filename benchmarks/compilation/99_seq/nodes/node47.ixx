export module arc.bench.compile99_seq.node47;

import arc;
export import arc.bench.compile99_seq.trait.trait46;
export import arc.bench.compile99_seq.trait.trait47;

namespace arc::bench::compile99_seq {

export
struct Node47 : arc::Node
{
    using Depends = arc::Depends<trait::Trait46>;
    using Traits = arc::Traits<Node47, trait::Trait47>;

    int impl(this auto const& self, trait::Trait47::get)
    {
        return self.i + self.getNode(trait::trait46).get();
    }

    Node47() = default;
    int i = 47;
};

}
