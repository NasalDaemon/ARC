module arc.bench.compile99.node84:impl;
import arc.bench.compile99.node84;

namespace arc::bench::compile99 {

template<class Context>
int Node84::Node<Context>::impl(trait::Trait84::get) const
{
    return i + getNode(trait::trait83).get();
}

}
