export module examples.greet.alice;

import examples.greet.traits;
import arc;
import std;

namespace examples::greet {

// Alice is a standard node implementing Greeter and Responder
export struct Alice
{
    template<class Context> // Context injected by ARC into the node's state
    struct Node : arc::Node
    {
        // Declares dependency on the Responder trait (provided by another node)
        using Depends = arc::Depends<trait::Responder>;

        // Declares which traits this node implements
        using Traits = arc::Traits<trait::Greeter, trait::Responder>;

        void impl(trait::Responder::respondTo, std::string_view name) const
        {
            std::println("Well met, {}. I am Alice of {} years!", name, age);
        }

        void impl(trait::Greeter::greet) const
        {
            std::println("Hello from Alice! I am {} years old.", age);
            // Resolve dependency (on Bob) using the injected `Context` template parameter
            getNode(trait::responder).respondTo("Alice");
            // The line above can be inlined by the compiler,
            // as getNode and respondTo are both direct calls
        }

        explicit Node(int age) : age(age) {}
        int age; // State specific to this node
    };
};

}
