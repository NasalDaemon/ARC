module arc.bench.compile99.node74:impl;
import arc.bench.compile99.node74;

namespace arc::bench::compile99 {

template<class Context>
int Node74::Node<Context>::impl(Trait74::get) const
{
    return i + getNode(trait73).get();
}

} // namespace arc::bench::compile99
