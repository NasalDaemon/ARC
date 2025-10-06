module arc.bench.compile99.node19:impl;
import arc.bench.compile99.node19;

namespace arc::bench::compile99 {
template<class Context>
int Node19::Node<Context>::impl(trait::Trait19::get) const
{
    return i + getNode(trait::trait18).get();
}
}
