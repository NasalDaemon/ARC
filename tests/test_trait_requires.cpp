#include "arc/doctest_prelude.h"
#include "arc/macros.hpp"

#if !ARC_IMPORT_STD
#include <concepts>
#include <cstddef>
#include <string>
#include <string_view>
#endif

import arc.tests.trait_requires;
import arc.tests.trait_requires.literals;
import arc;

#include "arc/doctest.h"

namespace arc::tests::trait_requires {

/* arc-begin

export module arc.tests.trait_requires;

namespace arc::tests::trait_requires {

trait SignedValue [Types]
{
    type Value
    requires std::signed_integral<typename Types::Value>
    requires (sizeof(typename Types::Value) >= 2)

    get() const -> Types::Value
}

trait Keyed [Types]
{
    type Key
    requires requires { typename Types::Key::Hash; Types::Key::null(); }

    key() const -> Types::Key
}

trait Scaled [Types]
{
    type Scalar
    requires requires (typename Types::Scalar s) { { s.scaled(2) } -> std::same_as<typename Types::Scalar>; }

    scalar() const -> Types::Scalar
}

trait Buffered [Types]
{
    type Buffer
    requires requires (typename Types::Buffer b, typename Types::Buffer const cb, int i) {
        { b.push(i) } -> std::same_as<void>;
        requires requires { { cb.size() } -> std::convertible_to<std::size_t>; };
    }

    buffer() -> Types::Buffer&
}

trait Mutating
{
    requires requires {
        // impl_.m( is called on the mutable implementation
        impl_.m();
    }

    m() -> void
}

trait Counting
{
    requires std::same_as<decltype(const_.n()), int>

    n() const -> int
}

} // namespace arc::tests::trait_requires

arc-end */

namespace types {

struct Key
{
    struct Hash {};
    static Key null() { return {}; }
};

struct KeyWithoutNull
{
    struct Hash {};
};

struct Scalar
{
    Scalar scaled(int) const { return *this; }
};

struct DoubleScalar
{
    double scaled(int) const { return 0; }
};

struct Buffer
{
    void push(int) {}
    std::size_t size() const { return 0; }
};

struct NamedSizeBuffer
{
    void push(int) {}
    std::string size() const { return "none"; }
};

struct WithoutM {};

struct LongN
{
    long n() const { return 0; }
};

template<class K>
struct KeyTypes
{
    using Key = K;
};

template<class V>
struct ValueTypes
{
    using Value = V;
};

template<class S>
struct ScalarTypes
{
    using Scalar = S;
};

template<class B>
struct BufferTypes
{
    using Buffer = B;
};

} // namespace types

namespace node {

struct Good : arc::NodeImpl<SignedValue, Keyed, Scaled, Buffered, Mutating, Counting, Literals>
{
    struct Types
    {
        using Value = int;
        using Key = types::Key;
        using Scalar = types::Scalar;
        using Buffer = types::Buffer;
    };

    int get() const { return 3; }
    types::Key key() const { return {}; }
    types::Scalar scalar() const { return {}; }
    types::Buffer& buffer() { return buffer_; }
    void m() { ++calls; }
    int n() const { return calls; }
    int f(std::string_view s, char c, int n) { return static_cast<int>(s.size()) + c + n; }

    types::Buffer buffer_;
    int calls = 0;
};

} // namespace node

SCENARIO(R"(a trait's `requires <constraint>` holds only when the constraint is true)")
{
    GIVEN(R"(a trait requiring a signed Value of at least two bytes)")
    {
        THEN(R"(a signed int Value satisfies it)")
        {
            static_assert(SignedValue::Meta::satisfiesRequirements<node::Good, types::ValueTypes<int>>);
        }
        THEN(R"(an unsigned Value, valid in the constraint but not meeting it, is rejected)")
        {
            static_assert(not SignedValue::Meta::satisfiesRequirements<node::Good, types::ValueTypes<unsigned>>);
        }
        THEN(R"(a one-byte Value fails the parenthesised constraint)")
        {
            static_assert(not SignedValue::Meta::satisfiesRequirements<node::Good, types::ValueTypes<signed char>>);
        }
    }
}

SCENARIO(R"(a trait's `requires requires { ... }` holds when every requirement in the body is valid)")
{
    GIVEN(R"(a body without a parameter list, holding several requirements)")
    {
        THEN(R"(types meeting all of them satisfy it)")
        {
            static_assert(Keyed::Meta::satisfiesRequirements<node::Good, types::KeyTypes<types::Key>>);
        }
        THEN(R"(types missing one of them are rejected)")
        {
            static_assert(not Keyed::Meta::satisfiesRequirements<node::Good, types::KeyTypes<types::KeyWithoutNull>>);
        }
    }
    GIVEN(R"(a body with a parameter list)")
    {
        THEN(R"(types returning the required type satisfy it)")
        {
            static_assert(Scaled::Meta::satisfiesRequirements<node::Good, types::ScalarTypes<types::Scalar>>);
        }
        THEN(R"(types returning another type are rejected)")
        {
            static_assert(not Scaled::Meta::satisfiesRequirements<node::Good, types::ScalarTypes<types::DoubleScalar>>);
        }
    }
    GIVEN(R"(a body with nested braces: a compound requirement and a nested requires-expression)")
    {
        THEN(R"(types meeting the nested requirement satisfy it)")
        {
            static_assert(Buffered::Meta::satisfiesRequirements<node::Good, types::BufferTypes<types::Buffer>>);
        }
        THEN(R"(types failing only the nested requirement are rejected)")
        {
            static_assert(not Buffered::Meta::satisfiesRequirements<node::Good, types::BufferTypes<types::NamedSizeBuffer>>);
        }
    }
}

SCENARIO(R"(a trait's requirements on impl_ and const_ hold only for implementations meeting them)")
{
    GIVEN(R"(a validity check calling impl_.m(), behind a comment with an unbalanced bracket)")
    {
        THEN(R"(an implementation with m() satisfies it)")
        {
            static_assert(Mutating::Meta::satisfiesRequirements<node::Good, void>);
        }
        THEN(R"(an implementation without m() is rejected)")
        {
            static_assert(not Mutating::Meta::satisfiesRequirements<types::WithoutM, void>);
        }
    }
    GIVEN(R"(a constraint that const_.n() returns int)")
    {
        THEN(R"(an implementation whose n() returns int satisfies it)")
        {
            static_assert(Counting::Meta::satisfiesRequirements<node::Good, void>);
        }
        THEN(R"(an implementation whose n() returns another type is rejected)")
        {
            static_assert(not Counting::Meta::satisfiesRequirements<types::LongN, void>);
        }
    }
}

SCENARIO(R"(a node meeting every requirement form implements its traits in a graph)")
{
    GIVEN(R"(a graph of a node implementing traits with each requirement form)")
    {
        arc::test::Graph<node::Good> graph;

        WHEN(R"(it is viewed through its traits)")
        {
            auto signedValue = graph.asTrait(trait::signedValue);
            auto buffered = graph.asTrait(trait::buffered);
            graph.asTrait(trait::keyed).key();
            graph.asTrait(trait::scaled).scalar();
            graph.asTrait(trait::mutating).m();
            auto counting = graph.asTrait(trait::counting);

            THEN(R"(its methods are reachable)")
            {
                CHECK(signedValue.get() == 3);
                CHECK(&buffered.buffer() == &graph.node->buffer_);
                CHECK(counting.n() == 1);
            }
        }
    }
}

SCENARIO(R"(brackets and comment markers inside literals are part of the literal)")
{
    GIVEN(R"(a trait whose requirement, return type, default bodies and precondition hold such literals)")
    {
        arc::test::Graph<node::Good> graph;
        auto literals = graph.asTrait(trait::literals);

        THEN(R"(the requirement is satisfied and each literal is kept as written)")
        {
            static_assert(Literals::Meta::satisfiesRequirements<node::Good, void>);
            CHECK(literals.f("}", '(', 1'000) == 1 + '(' + 1000);
            CHECK(literals.url() == "http://x/*");
            static_assert(decltype(literals.slashes())::value == 3);
            CHECK(literals.isComment('*'));
        }
    }
}

} // namespace arc::tests::trait_requires
