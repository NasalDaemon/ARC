export module abc.traits;

namespace abc {

trait AliceRead [Types]
{
    get() const -> int

    requires requires { { const_.impl(get_c) } -> std::same_as<int>; }
    requires requires { typename Types::AliceType; }
}

trait AliceWrite [Types, R = Root]
{
    set(int value)

    type AliceType

    requires requires { impl_.impl(set_c, std::declval<int>()); }
    requires std::same_as<decltype(impl_.impl(set_c, std::declval<int>())), void>

    requires requires (int i) {
        { impl_.impl(set_c, i) } -> std::same_as<void>;
    }
}

trait Alice = AliceRead + AliceWrite

trait Bob [Types]
{
    requires requires { typename Types::BobType; }

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
