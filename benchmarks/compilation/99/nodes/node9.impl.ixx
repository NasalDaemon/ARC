module arc.bench.compile99.node9:impl;
import arc.bench.compile99.node9;

namespace arc::bench::compile99 {

template<class Context>
int Node9::Node<Context>::impl(trait::Trait9::get) const
{
    return i + getNode(trait::trait8).get();
}

}
