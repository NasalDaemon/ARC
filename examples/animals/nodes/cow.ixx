export module examples.animals.cow;

import examples.animals.traits;
import arc;

namespace examples::animals::node {

export struct Cow : arc::NodeImpl<Animal*>
{
    std::string impl(Animal::speak) const { return happy ? "moo" : "mmmooooo!"; }

    explicit Cow(bool happy) : happy(happy) {}
    bool happy;
};

}
