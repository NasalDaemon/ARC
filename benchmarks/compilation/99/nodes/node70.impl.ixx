module arc.bench.compile99.node70:impl;
import arc.bench.compile99.node70;

namespace arc::bench::compile99 {

template<class Context>
int Node70::Node<Context>::impl(Trait70::get) const
{
    return i + getNode(trait69).get();
}

}
