export module arc.tests.domain;

namespace arc::tests::domain {

trait trait::Domain
{
    get()
}

trait trait::Parent
{
    get()
}

trait trait::Unary
{
    get()
}

trait trait::Unary2
{
    get()
}

trait trait::Unary3
{
    get()
}

domain Domain [R = Root]
{
    nexus = R::Nexus
    DOMAIN1 = Domain1
    DOMAIN2 = Domain1
    unary1 = R::Unary
    unary2 = R::Unary

    [trait::Domain]
    .. --> nexus --> DOMAIN1 --> DOMAIN2
    // .. --> unary1
    // .. --> DOMAIN1
    // unary1 --> DOMAIN1
    // DOMAIN1 --> unary1
    [trait::Parent]
    .. <-- nexus <-- DOMAIN1
    // .. <-- unary1
    // .. <-- DOMAIN1

    [trait::Unary]
    nexus --> unary1
    nexus <-- unary1

    [trait::Unary2]
    unary1 <<<--->>> unary2

    [trait::Unary3]
    unary1 <<<--->>> unary2
    // unary1 <<<--->>> unary2

    // [trait::Unary] nexus
}

domain Domain1 [R = Root]
{
    nexus = R::Nexus1
    DOMAIN2 = Domain2
}

domain Domain2 [R = Root]
{
    nexus = R::Nexus2
    DOMAIN3 = Domain3
}

domain Domain3 [R = Root]
{
    nexus = R::Nexus3
}

}
