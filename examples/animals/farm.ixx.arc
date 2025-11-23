export module examples.animals.farm;

import examples.animals.farmer;
import examples.animals.cow;
import examples.animals.sheep;
import examples.animals.traits;

cluster examples::animals::Farm
{
    farmer = Farmer
    animal = arc::Union<Cow, Sheep>

    [trait::Animal]
    farmer --> animal
}
