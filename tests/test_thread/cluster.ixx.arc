export module arc.tests.thread.cluster;

export import arc.tests.thread.traits;
export import arc.tests.thread.test_node;

namespace arc::tests::thread {

cluster Cluster
{
    a = TestNode<trait::A> : arc::OnThread<1>
    b = TestNode<trait::B> : arc::OnThread<2>
    c = TestNode<trait::C> : arc::OnThread<3>

    dynA = TestNode<trait::A> : arc::OnDynThread
    dynB = TestNode<trait::B> : arc::OnDynThread
    dynC = TestNode<trait::C> : arc::OnDynThread

    [trait::A] a <-- b, c
    [trait::B] b <-- a, c
    [trait::C] c <-- a, b

    [trait::A] dynA <-- dynB, dynC
    [trait::B] dynB <-- dynA, dynC
    [trait::C] dynC <-- dynA, dynB
}

}
