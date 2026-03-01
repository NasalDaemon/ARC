module arc.bench.compile99.node43:impl;
import arc.bench.compile99.node43;

namespace arc::bench::compile99 {

template<class Context>
int Node43::Node<Context>::impl(Trait43::get) const
{
    return i + getNode(trait42).get();
}

}
