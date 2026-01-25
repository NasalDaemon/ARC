import examples.greet.forum;
import examples.greet.traits;
import arc;

using namespace examples::greet;

int main()
{
    // Instantiate the graph: all nodes with dependencies resolved at compile time
    arc::Graph<Forum> graph{
        .alice{29},
        .bob{30},
    };
    // Graph is a single object on the stack containing all nodes
    static_assert(sizeof(graph) == 2 * sizeof(int));

    // Access nodes through their traits
    graph.alice.asTrait(trait::greeter).greet();
    // Output:
    // Hello from Alice! I am 29 years old.
    // Well met, Alice. I am Alice of 29 years!

    graph.bob.asTrait(trait::greeter).greet();
    // Output:
    // Hello from Bob! I am 30 years old.
    // Well met, Bob. I am Alice of 29 years!

    return 0;
}
