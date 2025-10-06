export module abc.traits;

namespace abc::trait {

trait AliceRead [Types]
{
    get() const -> int

    requires { const_.impl(get_c) } -> std::same_as<int>
    requires typename Types::AliceType
}

trait AliceWrite [Types, R = Root]
{
    set(int value)

    //requires typename R::AliceType;
    //requires { const_.asdasd() }
    type AliceType

    requires impl_.impl(set_c, std::declval<int>())
    requires { impl_.impl(set_c, std::declval<int>()) } -> std::same_as<void>

    requires (int i) {
        { impl_.impl(set_c, i) } -> std::same_as<void>
    }
}

trait Alice = AliceRead + AliceWrite

trait Bob [Types]
{
    requires typename Types::BobType

    template<class... Ts>
    get(Ts const&&... i) const -> Types::BobType
    set(Types::BobType value) -> void
}

trait Charlie [Types]
{
    get() const -> Types::CharlieType
}

trait Charlie2 = Charlie
trait Charlie3 = Charlie

trait Ellie
{
    type EllieType

    get() const -> std::same_as<int> auto
    hi(...)
}

trait Ellie2 = Ellie
trait Ellie3 = Ellie

trait Visitable
{
    visit(int& counter) -> void
}

}
