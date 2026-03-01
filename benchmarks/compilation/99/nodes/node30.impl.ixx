module arc.bench.compile99.node30:impl;
import arc.bench.compile99.node30;

namespace arc::bench::compile99 {

template<class Context>
int Node30::Node<Context>::impl(Trait30::get) const
{
    return i + getNode(trait29).get();
}

}
