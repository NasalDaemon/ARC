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
        std::string speak() const override
        {
            return happy ? "moo" : "mmmooooo!";
        }

        void evolve() override
        {
            // Cows do not evolve
        }

        explicit Node(bool happy) : happy(happy) {}
        bool happy;
    };
};

}
