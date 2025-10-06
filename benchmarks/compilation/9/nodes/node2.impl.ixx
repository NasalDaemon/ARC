module arc.bench.compile9.node2:impl;

import arc.bench.compile9.node2;
import arc;

namespace arc::bench::compile9 {

template<class Context>
int Node2::Node<Context>::impl(trait::Trait2::get) const
{
    return i + getNode(trait::trait1).get();
}

}
