import examples.animals.farm;
import examples.animals.cow;
import examples.animals.sheep;
import examples.animals.traits;
import arc;
import std;

using namespace examples::animals;

int main()
{
    arc::Graph<Farm> graph{
        .animal{arc::withFactory,
            [&](auto construct) {
                return construct(std::in_place_type<Cow>, true);
            }
        },
    };

    graph.farmer->greetAnimal();  // Animal says: moo

    // Change impl at runtime by index
    graph.animal->emplace<0>(false);
    std::println("Unhappy cow says {}", graph.farmer->getNode(trait::animal).speak());

    // Change impl at runtime by type
    graph.animal->emplace<Sheep>();
    graph.farmer->greetAnimal();  // Animal says: baa!

    return 0;
}
