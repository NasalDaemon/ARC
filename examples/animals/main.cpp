import examples.animals.farm;
import examples.animals.cow;
import examples.animals.sheep;
import examples.animals.traits;
import arc;
import std;

using namespace examples::animals;

int main()
{
    arc::Graph<cluster::Farm> graph{
        .animal{std::in_place_type<node::Cow>, true},
    };

    graph.farmer->greetAnimal();  // Animal says: moo

    // Change impl at runtime by index
    graph.animal->emplace<0>(false);
    std::println("Unhappy cow says {}", graph.farmer.getNode(trait::animal).speak());

    // Change impl at runtime by type
    graph.animal->emplace<node::Sheep>();
    graph.farmer->greetAnimal();  // Animal says: baa!

    return 0;
}
