module arc.bench.compile99.node11:impl;
import arc.bench.compile99.node11;

namespace arc::bench::compile99 {
template<class Context>
int Node11::Node<Context>::impl(Trait11::get) const
{
    return i + getNode(trait10).get();
}
}
