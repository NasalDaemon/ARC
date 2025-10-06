module arc.bench.compile99.node80:impl;
import arc.bench.compile99.node80;

namespace arc::bench::compile99 {

template<class Context>
int Node80::Node<Context>::impl(trait::Trait80::get) const
{
    return i + getNode(trait::trait79).get();
}

}
