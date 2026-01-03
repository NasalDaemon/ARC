export module examples.greet.traits;

namespace examples::greet::trait {

// Traits define interfaces between nodes
trait Greeter
{
    greet() const
}

trait Responder
{
    respondTo(std::string_view name) const
}

}
