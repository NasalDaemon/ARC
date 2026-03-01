module arc.bench.compile99.node60:impl;
import arc.bench.compile99.node60;

namespace arc::bench::compile99 {

template<class Context>
int Node60::Node<Context>::impl(Trait60::get) const
{
    return i + getNode(trait59).get();
}

}
