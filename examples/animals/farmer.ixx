export module examples.animals.farmer;

import examples.animals.traits;
import arc;

namespace examples::animals {

export struct Farmer : arc::Node
{
    using Depends = arc::Depends<trait::Animal>;

    using Traits = arc::Traits<Farmer>;

    void greetAnimal(this auto const& self)
    {
        auto animal = self.getNode(trait::animal);
        std::println("Animal says: {}", animal.speak());
    }
};

}
