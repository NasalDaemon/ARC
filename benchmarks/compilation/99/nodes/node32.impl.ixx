module arc.bench.compile99.node32:impl;
import arc.bench.compile99.node32;

namespace arc::bench::compile99 {

template<class Context>
int Node32::Node<Context>::impl(Trait32::get) const
{
    return i + getNode(trait31).get();
}

}
