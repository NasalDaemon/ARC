export module arc.tests.repeater.traits;

namespace arc::tests::repeater::trait {

trait Trait
{
    defer(int& i) const -> void
    function(int& i) const -> void

    // defer(...)
    // function(...)
}

}
