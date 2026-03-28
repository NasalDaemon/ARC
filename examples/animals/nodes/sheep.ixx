export module examples.animals.sheep;

import examples.animals.traits;
import arc;

namespace examples::animals::node {

export struct Sheep : arc::NodeImpl<Animal*>
{
    std::string impl(Animal::speak) const { return "baa!"; }
};

} // namespace examples::animals::node
