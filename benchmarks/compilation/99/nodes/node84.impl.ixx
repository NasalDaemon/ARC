module arc.bench.compile99.node84:impl;
import arc.bench.compile99.node84;

namespace arc::bench::compile99 {

template<class Context>
int Node84::Node<Context>::impl(Trait84::get) const
{
    return i + getNode(trait83).get();
}

}
