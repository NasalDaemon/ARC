export module arc.tests.repeater.traits;

namespace arc::tests::repeater {

trait Source
{
    defer(int& i) -> void
}

trait Target
{
    function(int& i) -> void
}

}
