export module examples.animals_virtual.cow;

import examples.animals_virtual.animal;
import examples.animals_virtual.traits;
import arc;

namespace examples::animals_virtual {

export struct Cow
{
    template<class Context>
    struct Node final : IAnimal
    {
        using Traits = arc::TraitsFrom<Node, IAnimal>;

        std::string impl(trait::Animal::speak) const override
        {
            return happy ? "moo" : "mmmooooo!";
        }

        void impl(trait::Animal::evolve) override
        {
            // Cows do not evolve
        }

        explicit Node(bool happy) : happy(happy) {}
        bool happy;
    };
};

}
