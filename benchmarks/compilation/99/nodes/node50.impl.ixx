module arc.bench.compile99.node50:impl;
import arc.bench.compile99.node50;

namespace arc::bench::compile99 {

template<class Context>
int Node50::Node<Context>::impl(trait::Trait50::get) const
{
    return i + getNode(trait::trait49).get();
}

} // namespace arc::bench::compile99
