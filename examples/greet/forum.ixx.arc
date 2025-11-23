export module examples.greet.forum;

import examples.greet.alice;
import examples.greet.bob;
import examples.greet.traits;

// Cluster wires nodes together, satisfying dependencies
cluster examples::greet::Forum
{
    alice = Alice
    bob = Bob

    [trait::Responder]
    alice --> bob  // alice depends on bob for trait::Responder
    alice <-- bob  // bob depends on alice for trait::Responder
    // Can also be expressed simply as:
    // alice <-> bob
}
