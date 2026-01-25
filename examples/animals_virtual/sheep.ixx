export module examples.animals_virtual.sheep;

import examples.animals_virtual.animal;
import examples.animals_virtual.traits;
import examples.animals_virtual.goat;
import arc;

namespace examples::animals_virtual {

export struct Sheep {
    template<class Context>
    struct Node final : IAnimal
    {
        std::string speak() const override
        {
            return black ? "barbar" : "baa!";
        }

        void evolve() override
        {
            auto handle = exchangeImpl<Goat>();
            // In the context of a global scheduler, this exchange will be deferred until
            // all scheduler tasks have been drained and the scheduler is in exclusive mode (idle).
            // For the sake of simplicity, we assume there is no global scheduler here.
            if (handle.deferred())
                throw std::runtime_error("Sheep::evolve exchangeImpl was not expected to be deferred");

            // This node is still alive in a detached state
            // It can call into itself and other nodes, but other nodes can't reach it
            std::println("{}", speak());
            // prints: barbar or baa!

            std::println("{}", handle.getNext()->speak());
            // prints: meeh!
        }

        Node(bool black = false) : black(black) {}
        bool black;
    };
};

}
