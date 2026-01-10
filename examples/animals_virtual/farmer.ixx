export module examples.animals_virtual.farmer;

import examples.animals_virtual.traits;
import arc;
import std;

namespace examples::animals_virtual {

export struct Farmer : arc::NodeUses<trait::Animal>
{
    void greetAnimal(this auto const& self)
    {
        std::println("Animal says {}", self.getAnimal().speak());
    }
};

}
