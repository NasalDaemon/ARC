export module arc.tests.thread.traits;

namespace arc::tests::thread::trait {

trait Trait
{
    getA() const -> int
    getB() const -> int
    getC() const -> int
}
trait A = Trait
trait B = Trait
trait C = Trait

}
