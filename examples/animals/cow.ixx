export module examples.animals.cow;

import examples.animals.traits;
import arc;

namespace examples::animals {

export struct Cow : arc::Node
{
    using Traits = arc::Traits<Cow, trait::Animal>;

    std::string impl(trait::Animal::speak) const { return happy ? "moo" : "mmmooooo!"; }

    explicit Cow(bool happy) : happy(happy) {}
    bool happy;
};

}
