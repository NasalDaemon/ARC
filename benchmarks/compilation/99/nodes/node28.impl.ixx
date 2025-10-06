module arc.bench.compile99.node28:impl;
import arc.bench.compile99.node28;

namespace arc::bench::compile99 {

template<class Context>
int Node28::Node<Context>::impl(trait::Trait28::get) const
{
    return i + getNode(trait::trait27).get();
}

} // namespace arc::bench::compile99
