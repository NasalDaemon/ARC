export module arc.bench.compile9.graph;

import arc.bench.compile9.node1;
import arc.bench.compile9.node2;
import arc.bench.compile9.node3;
import arc.bench.compile9.node4;
import arc.bench.compile9.node5;
import arc.bench.compile9.node6;
import arc.bench.compile9.node7;
import arc.bench.compile9.node8;
import arc.bench.compile9.node9;

cluster arc::bench::compile9::Graph
{
    node1 = Node1
    node2 = Node2
    node3 = Node3
    node4 = Node4
    node5 = Node5
    node6 = Node6
    node7 = Node7
    node8 = Node8
    node9 = Node9

    // Dependencies (each nodeN depends on previous node's trait)
    [Trait1]  node2  --> node1
    [Trait2]  node3  --> node2
    [Trait3]  node4  --> node3
    [Trait4]  node5  --> node4
    [Trait5]  node6  --> node5
    [Trait6]  node7  --> node6
    [Trait7]  node8  --> node7
    [Trait8]  node9  --> node8
}
