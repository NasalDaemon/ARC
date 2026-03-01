export module examples.greet.node.bob;

import examples.greet.traits;
import arc;
import std;

namespace examples::greet::node {

// Bob is a shorthand node with contextless state
// Context is injected into methods by ARC via deducing-this parameter instead
export struct Bob : arc::Node::
    Uses<trait::Responder>:: // provides arc::Depends<...> list
    Impl<trait::Greeter, trait::Responder> // provides arc::Traits<...> list
{
    // impl(trait::Responder::method, ...) redirects here via Impl<..., trait::Responder>
    void respondTo(std::string_view name) const
    {
        std::println("Well met, {}. I am Bob of {} years!", name, age);
    }

    // impl(trait::Greeter::method, ...) redirects here via Impl<trait::Greeter, ...>
    void greet(this auto const& self) // deducing-this `self` parameter has the node context
    {
        std::println("Hello from Bob!");
        // Uses<trait::Responder> provides `getResponder()` aka `getNode(trait::responder)`
        self.getResponder().respondTo("Bob");
        // The line above will be inlined by the compiler
    }

    explicit Bob(int age) : age(age) {}
    int age; // State specific to this node
};

}
