module arc.bench.compile99.node23:impl;
import arc.bench.compile99.node23;

namespace arc::bench::compile99 {

template<class Context>
int Node23::Node<Context>::impl(trait::Trait23::get) const
{
    return i + getNode(trait::trait22).get();
}

}
