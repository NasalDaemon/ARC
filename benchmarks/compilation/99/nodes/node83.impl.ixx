module arc.bench.compile99.node83:impl;
import arc.bench.compile99.node83;

namespace arc::bench::compile99 {

template<class Context>
int Node83::Node<Context>::impl(Trait83::get) const
{
    return i + getNode(trait82).get();
}

} // namespace arc::bench::compile99
