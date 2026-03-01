export module examples.greet.cluster.forum;

import examples.greet.node.alice;
import examples.greet.node.bob;
import examples.greet.traits;

// Cluster wires nodes together, satisfying dependencies
cluster examples::greet::Forum
{
    alice = node::Alice
    bob = node::Bob

    [trait::Responder]
    alice --> bob  // alice depends on bob for trait::Responder
    alice <-- bob  // bob depends on alice for trait::Responder
    // Can also be expressed simply as:
    // alice <-> bob
}
