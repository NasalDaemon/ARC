module arc.bench.compile99.node76:impl;
import arc.bench.compile99.node76;

namespace arc::bench::compile99 {

template<class Context>
int Node76::Node<Context>::impl(Trait76::get) const
{
    return i + getNode(trait75).get();
}

}
