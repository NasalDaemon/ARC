export module examples.animals.farm;

import examples.animals.farmer;
import examples.animals.cow;
import examples.animals.sheep;
import examples.animals.traits;

cluster examples::animals::Farm
{
    farmer = node::Farmer
    animal = arc::Union<node::Cow, node::Sheep>

    [Animal]
    farmer --> animal
}
