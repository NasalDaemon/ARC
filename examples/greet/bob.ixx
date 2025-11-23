export module examples.greet.bob;

import examples.greet.traits;
import arc;
import std;

namespace examples::greet {

// Bob is full node with contextful state
export struct Bob
{
    template<class Context>
    struct Node : arc::Node
    {
        using Depends = arc::Depends<trait::Responder>;
        using Traits = arc::Traits<Node, trait::Greeter, trait::Responder>;

        Node(int age) : age(age) {}

        void impl(trait::Greeter::greet) const
        {
            std::println("Hello from Bob!");
            getNode(trait::responder).respondTo("Bob");
        }

        void impl(trait::Responder::respondTo, std::string_view name) const
        {
            std::println("Well met, {}. I am Bob of {} years!", name, age);
        }

        int age; // State specific to this node
    };
};

}
