module arc.bench.compile99.node80:impl;
import arc.bench.compile99.node80;

namespace arc::bench::compile99 {

template<class Context>
int Node80::Node<Context>::impl(Trait80::get) const
{
    return i + getNode(trait79).get();
}

}
