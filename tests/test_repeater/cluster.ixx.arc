export module arc.tests.repeater.cluster;

export import arc.tests.repeater.traits;
export import arc.tests.repeater.test_node;

namespace arc::tests::repeater {

cluster Cluster
{
    a = TestNode
    b = TestNode
    c = TestNode

    [trait::Trait]
    .., a --> b, c
}

}
