module arc.bench.compile99.node24:impl;
import arc.bench.compile99.node24;

namespace arc::bench::compile99 {

template<class Context>
int Node24::Node<Context>::impl(Trait24::get) const
{
    return i + getNode(trait23).get();
}

}
