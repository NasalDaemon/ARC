export module abc.graph.alicebob;

export import abc.traits;
import abc.graph.charlie;

import abc.alice;
import abc.bob;
import abc.ellie;

namespace abc {

cluster AliceBob
{
    alice = node::Alice//<p.operator()<int>(0)>
    bob = node::Bob
    charlie = cluster::CharlieCluster
    ellie = node::Ellie

    using a = trait::Alice, b = trait::Bob, c = trait::Charlie,
          e = trait::Ellie, e2 = trait::Ellie2, e3 = trait::Ellie3

    [a]  alice   <--      .., bob, charlie
    [b]  bob     <--      .., alice, charlie
    [c]  charlie <--      .., alice, bob, ellie
    [e]  ellie   <--      .., bob, charlie
         ellie   <-- (e2) alice
    [e3] ellie   <--      alice
}

}
