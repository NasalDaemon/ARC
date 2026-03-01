export module examples.animals.farmer;

import examples.animals.traits;
import arc;

namespace examples::animals::node {

export struct Farmer : arc::NodeUses<Animal>
{
    void greetAnimal(this auto const& self)
    {
        auto animal = self.getAnimal();
        std::println("Animal says: {}", animal.speak());
    }
};

}
