module arc.bench.compile99.node34:impl;
import arc.bench.compile99.node34;

namespace arc::bench::compile99 {

template<class Context>
int Node34::Node<Context>::impl(Trait34::get) const
{
    return i + getNode(trait33).get();
}

} // namespace arc::bench::compile99
