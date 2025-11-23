export module examples.animals_virtual.farm;

import examples.animals_virtual.farmer;
import examples.animals_virtual.animal;
import examples.animals_virtual.traits;

cluster examples::animals_virtual::Farm
{
    farmer = Farmer
    animal = arc::Virtual<IAnimal>

    [trait::Animal]
    farmer --> animal
}
