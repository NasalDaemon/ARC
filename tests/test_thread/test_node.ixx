module;
#if !ARC_IMPORT_STD
#include <type_traits>
#endif
export module arc.tests.thread.test_node;

import arc.tests.thread.poster;
import arc.tests.thread.traits;
import arc;
#if ARC_IMPORT_STD
import std;
#endif

export namespace arc::tests::thread {

template<class Trait>
struct TestNode : arc::Node
{
    struct Interface : arc::DetachedInterface
    {
        int impl(this auto const& self, trait::Trait::getA)
        {
            if constexpr (std::is_same_v<Trait, trait::A>)
                return self->i;
            else
                return self.getNode(trait::a).getA();
        }
        int impl(this auto const& self, trait::Trait::getB method)
        {
            if constexpr (std::is_same_v<Trait, trait::B>)
                return self->i;
            else
                return self.getNode(trait::b, future).impl(method).get();
        }
        int impl(this auto const& self, trait::Trait::getC)
        {
            if constexpr (std::is_same_v<Trait, trait::C>)
                return self->i;
            else
                return self.getNode(trait::c).getC();
        }
    };

    using Traits = arc::Traits<TestNode(Interface), trait::A, trait::B, trait::C>;

    TestNode(int i) : i(i) {}

    int i;
};

} // namespace arc::tests::thread
