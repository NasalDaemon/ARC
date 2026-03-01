module arc.bench.compile99.node18:impl;
import arc.bench.compile99.node18;

namespace arc::bench::compile99 {
template<class Context>
int Node18::Node<Context>::impl(Trait18::get) const
{
    return i + getNode(trait17).get();
}
}
