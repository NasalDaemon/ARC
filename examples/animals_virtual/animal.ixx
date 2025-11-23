export module examples.animals_virtual.animal;

import examples.animals_virtual.traits;
import arc;
import std;

namespace examples::animals_virtual {

export struct IAnimal : arc::INode
{
    using Traits = arc::Traits<IAnimal, trait::Animal>;

    virtual std::string impl(trait::Animal::speak) const = 0;
    virtual void impl(trait::Animal::evolve) = 0;
};

}
