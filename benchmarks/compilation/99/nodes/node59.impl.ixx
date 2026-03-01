module arc.bench.compile99.node59:impl;
import arc.bench.compile99.node59;

namespace arc::bench::compile99 {

template<class Context>
int Node59::Node<Context>::impl(Trait59::get) const
{
    return i + getNode(trait58).get();
}

} // namespace arc::bench::compile99
