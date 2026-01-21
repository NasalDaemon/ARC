export module arc.tests.repeater.cluster;

export import arc.tests.repeater.traits;
export import arc.tests.repeater.test_node;

namespace arc::tests::repeater {

cluster Cluster
{
    a = SourceNode
    b = TargetNode
    c = TargetNode

    [trait::Target]
    .., a --> {b, c}
}

}
