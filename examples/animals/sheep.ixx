export module examples.animals.sheep;

import examples.animals.traits;
import arc;

namespace examples::animals {

export struct Sheep : arc::Node
{
    using Traits = arc::Traits<Sheep, trait::Animal>;

    std::string impl(trait::Animal::speak) const { return "baa!"; }
};

}
