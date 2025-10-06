module arc.bench.compile99.node96:impl;
import arc.bench.compile99.node96;

namespace arc::bench::compile99 {

template<class Context>
int Node96::Node<Context>::impl(trait::Trait96::get) const
{
    return i + getNode(trait::trait95).get();
}

} // namespace arc::bench::compile99
