export module examples.greet.alice;

import examples.greet.traits;
import arc;
import std;

namespace examples::greet {

// Alice is a short-hand node with contextless state
// It implements Greeter and Responder
export struct Alice : arc::Node
{
    // Declares dependency on the Responder trait (provided by another node)
    using Depends = arc::Depends<trait::Responder>;

    // Declares which traits this node implements
    using Traits = arc::Traits<Alice, trait::Greeter, trait::Responder>;

    void impl(this auto const& self, trait::Greeter::greet)
    {
        std::println("Hello from Alice! I am {} years old.", self.age);
        // Resolve dependency via explicit object parameter `self`, containing the node's context
        self.getNode(trait::responder).respondTo("Alice");
        // The line above can be inlined by the compiler, as getNode and respondTo are both direct calls
    }

    void impl(trait::Responder::respondTo, std::string_view name) const
    {
        std::println("Well met, {}. I am Alice of {} years!", name, age);
    }

    explicit Alice(int age) : age(age) {}
    int age; // State specific to this node
};

}
