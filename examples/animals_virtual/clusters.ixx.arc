export module examples.animals_virtual.farm;

import examples.animals_virtual.farmer;
import examples.animals_virtual.animal;
import examples.animals_virtual.traits;

cluster examples::animals_virtual::Farm
{
    farmer = node::Farmer
    animal = arc::Virtual<node::IAnimal>

    [Animal]
    farmer --> animal
}
