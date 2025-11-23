export module examples.animals_virtual.farmer;

import examples.animals_virtual.traits;
import arc;
import std;

namespace examples::animals_virtual {

export struct Farmer : arc::Node
{
    using Depends = arc::Depends<trait::Animal>;

    using Traits = arc::Traits<Farmer>;

    void greetAnimal(this auto const& self)
    {
        std::println("Animal says {}", self.getNode(trait::animal).speak());
    }
};

}
