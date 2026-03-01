module arc.bench.compile99.node95:impl;
import arc.bench.compile99.node95;

namespace arc::bench::compile99 {

template<class Context>
int Node95::Node<Context>::impl(Trait95::get) const
{
    return i + getNode(trait94).get();
}

} // namespace arc::bench::compile99
