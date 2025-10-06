module arc.bench.compile99.node12:impl;
import arc.bench.compile99.node12;

namespace arc::bench::compile99 {
template<class Context>
int Node12::Node<Context>::impl(trait::Trait12::get) const
{
    return i + getNode(trait::trait11).get();
}
}
