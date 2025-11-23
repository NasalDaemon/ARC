export module examples.animals_virtual.animal_facade;

import examples.animals_virtual.animal;
import examples.animals_virtual.cow;
import examples.animals_virtual.traits;
import arc;
import std;

namespace examples::animals_virtual {

export struct IAnimalFacade
{
    template<class Context>
    struct Node final : IAnimal
    {
        using Traits = arc::TraitsFrom<Node, IAnimal>;

        // Forward to adapted node
        std::string impl(trait::Animal::speak) const override
        {
            return getNode(trait::animal).speak();
        }

        // As a dynamic node, this facade can swap the implementation hosted by arc::Virtual
        void impl(trait::Animal::evolve) override
        {
            auto handle = exchangeImpl<Cow>(true);
            if (handle.deferred())
                throw std::runtime_error("IAnimalFacade::evolve exchangeImpl was not expected to be deferred");

            // This node is still alive in a detached state
            // It can call into itself and other nodes, but other nodes can't reach it
            std::println("{}", asTrait(trait::animal).speak());
            // prints: yip! (assuming it adapted Fox)

            std::println("{}", handle.getNext()->asTrait(trait::animal).speak());
            // prints: moo!
        }
    };
};

}
