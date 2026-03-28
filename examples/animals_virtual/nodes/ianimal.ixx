export module examples.animals_virtual.animal;

import examples.animals_virtual.traits;
import arc;
import std;

namespace examples::animals_virtual::node {

export struct IAnimal : arc::INodeImpl<Animal>
{
    // INodeImpl redirects impl(method, ...) to method(...)

    // Virtual methods to be implemented by derived classes
    virtual std::string speak() const = 0;
    virtual void evolve() = 0;
};

} // namespace examples::animals_virtual::node
