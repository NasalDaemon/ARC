module arc.bench.compile99.node5:impl;

import arc.bench.compile99.node5;

namespace arc::bench::compile99 {

template<class Context>
int Node5::Node<Context>::impl(trait::Trait5::get) const
{
    return i + getNode(trait::trait4).get();
}

}
