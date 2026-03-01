module arc.bench.compile99.node82:impl;
import arc.bench.compile99.node82;

namespace arc::bench::compile99 {

template<class Context>
int Node82::Node<Context>::impl(Trait82::get) const
{
    return i + getNode(trait81).get();
}

} // namespace arc::bench::compile99
