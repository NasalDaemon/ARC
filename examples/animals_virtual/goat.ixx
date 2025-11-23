export module examples.animals_virtual.goat;

import examples.animals_virtual.animal;
import examples.animals_virtual.traits;
import examples.animals_virtual.cow;
import arc;

namespace examples::animals_virtual {

export struct Goat {
    template<class Context>
    struct Node final : IAnimal
    {
        using Traits = arc::TraitsFrom<Node, IAnimal>;

        std::string impl(trait::Animal::speak) const override { return "meeh!"; }

        void impl(trait::Animal::evolve) override
        {
            auto handle = exchangeImpl<Cow>(false);
            if (handle.deferred())
                throw std::runtime_error("Goat::evolve exchangeImpl was not expected to be deferred");

            // This node is still alive in a detached state
            // It can call into itself and other nodes, but other nodes can't reach it
            std::println("{}", asTrait(trait::animal).speak());
            // prints: meeh!

            std::println("{}", handle.getNext()->asTrait(trait::animal).speak());
            // prints: mmmooooo!
        }
    };
};

}
