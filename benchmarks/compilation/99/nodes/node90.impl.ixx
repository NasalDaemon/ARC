module arc.bench.compile99.node90:impl;
import arc.bench.compile99.node90;

namespace arc::bench::compile99 {

template<class Context>
int Node90::Node<Context>::impl(Trait90::get) const
{
    return i + getNode(trait89).get();
}

} // namespace arc::bench::compile99
