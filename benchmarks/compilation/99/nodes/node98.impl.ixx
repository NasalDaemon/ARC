module arc.bench.compile99.node98:impl;
import arc.bench.compile99.node98;

namespace arc::bench::compile99 {

template<class Context>
int Node98::Node<Context>::impl(Trait98::get) const
{
    return i + getNode(trait97).get();
}

} // namespace arc::bench::compile99
