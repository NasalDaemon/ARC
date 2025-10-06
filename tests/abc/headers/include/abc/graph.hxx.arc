#include "abc/alice.hpp"
#include "abc/bob.hpp"
#include "abc/charlie.hpp"
#include "abc/ellie.hpp"

namespace abc {

cluster CharlieCluster
{
    charlie = Charlie

    using a = trait::Alice, c = trait::Charlie
    [a <-> c] .. <-> charlie
}

cluster AliceBob
{
    alice = Alice
    bob = Bob
    charlie = CharlieCluster
    ellie = Ellie

    using a = trait::Alice, b = trait::Bob, c = trait::Charlie

    [a] alice   (trait::Alice)   <-- (trait::Alice)   .., bob, charlie
    [b] bob     (trait::Bob)     <-- (trait::Bob)     .., alice, charlie
    [c] charlie (trait::Charlie) <-- (trait::Charlie) .., alice, bob, ellie

    using e = trait::Ellie, e2 = trait::Ellie2, e3 = trait::Ellie3

    [e]  ellie   <--      .., alice, bob, charlie
         ellie   <-- (e2) alice
    [e3] ellie   <--      alice

}

}
