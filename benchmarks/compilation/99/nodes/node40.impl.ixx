module arc.bench.compile99.node40:impl;
import arc.bench.compile99.node40;

namespace arc::bench::compile99 {

template<class Context>
int Node40::Node<Context>::impl(Trait40::get) const
{
    return i + getNode(trait39).get();
}

}
