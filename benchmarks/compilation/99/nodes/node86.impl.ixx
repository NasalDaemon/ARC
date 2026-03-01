module arc.bench.compile99.node86:impl;
import arc.bench.compile99.node86;

namespace arc::bench::compile99 {

template<class Context>
int Node86::Node<Context>::impl(Trait86::get) const
{
    return i + getNode(trait85).get();
}

} // namespace arc::bench::compile99
