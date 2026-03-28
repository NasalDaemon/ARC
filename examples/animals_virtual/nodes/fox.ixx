export module examples.animals_virtual.fox;

import examples.animals_virtual.traits;
import arc;

namespace examples::animals_virtual::node {

// Fox is a static node which we can adapt to IAnimal using arc::Adapt<Fox, IAnimalFacade>
export struct Fox : arc::Node
{
    using Traits = arc::Traits<Animal>;

    std::string impl(Animal::speak) const { return "yip"; }

    void impl(Animal::evolve)
    {
        // static node cannot swap itself out inside a arc::Virtual (alas, `evolve` shouldn't really be a method in the trait)
    }
};

} // namespace examples::animals_virtual::node
