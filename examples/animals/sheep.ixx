export module examples.animals.sheep;

import examples.animals.traits;
import arc;

namespace examples::animals {

export struct Sheep : arc::NodeImpl<trait::Animal>
{
    std::string impl(trait::Animal::speak) const { return "baa!"; }
};

}
