import examples.animals_virtual.animal_facade;
import examples.animals_virtual.cow;
import examples.animals_virtual.farm;
import examples.animals_virtual.fox;
import examples.animals_virtual.sheep;
import examples.animals_virtual.traits;
import arc;
import std;

using namespace examples::animals_virtual;

enum class Animal
{
    Cow, Sheep
};

struct Args
{
    Animal animal = Animal::Cow;
};

Args parseArgs(int argc, char** argv)
{
    if (argc > 1)
    {
        std::string_view arg1 = argv[1];
        if (arg1 == "cow")
            return Args{Animal::Cow};
        else if (arg1 == "sheep")
            return Args{Animal::Sheep};
        else
        {
            std::println("Unknown animal: {}. Valid options are 'cow' or 'sheep'.", arg1);
            std::exit(1);
        }
    }
    return Args{Animal::Cow};
}

int main(int argc, char** argv)
{
    Args args = parseArgs(argc, argv);

    arc::Graph<Farm> graph{
        .animal{arc::withFactory,
            [&](auto construct) {
                if (args.animal == Animal::Cow)
                    return construct(std::in_place_type<Cow>, true);
                else
                    return construct(std::in_place_type<Sheep>);
            }}
    };

    graph.farmer->greetAnimal();

    auto animal = graph.farmer.getNode(trait::animal); // equivalent to graph.animal.asTrait(trait::animal)
    graph.animal->emplace<Sheep>(true);
    std::println("Black sheep says {}", animal.speak());

    // Sheep hotswaps with goat
    animal.evolve();
    std::println("Goat says {}", animal.speak());
    // Goat hotswaps with sad cow
    animal.evolve();
    std::println("Sad cow says {}", animal.speak());

    // Farmer's view of animal is always up-to-date
    // The `animal` variable is just a non-owning trait view of the arc::Virtual node in the graph
    std::println("Sad cow says {}", graph.farmer.getNode(trait::animal).speak());

    graph.animal->emplace<arc::Adapt<Fox, IAnimalFacade>>();
    std::println("Fox says {}", animal.speak());
    // IAnimalFacade hot-swaps with happy cow
    animal.evolve();
    std::println("Cow says {}", animal.speak());

    return 0;
}
