module arc.bench.compile9.node1:impl;

import arc.bench.compile9.node1;
import arc;

namespace arc::bench::compile9 {

template<class Context>
int Node1::Node<Context>::impl(trait::Trait1::get) const
{
    return i;
}

}
