#include "arc/doctest_prelude.h"
#include "arc/macros.hpp"

#if !ARC_IMPORT_STD
#include <string>
#include <type_traits>
#include <vector>
#endif

import arc;

#include "arc/doctest.h"

namespace arc::tests::function_ref {

namespace {

int twice(int x) { return 2 * x; }

int sum(FunctionRef<int(int)> f, std::vector<int> const& xs)
{
    int total = 0;
    for (int x : xs)
        total += f(x);
    return total;
}

} // namespace

TEST_CASE("FunctionRef is two pointers and trivially copyable")
{
    static_assert(sizeof(FunctionRef<void(int)>) == 2 * sizeof(void*));
    static_assert(std::is_trivially_copyable_v<FunctionRef<void(int)>>);
}

TEST_CASE("FunctionRef calls a capturing lambda, which sees its captures change")
{
    int offset = 1;
    auto add   = [&](int x) { return x + offset; };
    FunctionRef<int(int)> f{add};
    CHECK(f(1) == 2);
    offset = 10;
    CHECK(f(1) == 11);
}

TEST_CASE("FunctionRef calls a temporary lambda passed as an argument")
{
    CHECK(sum([](int x) { return x * x; }, {1, 2, 3}) == 14);
}

TEST_CASE("FunctionRef calls a mutable callable in place")
{
    struct Counter
    {
        int calls = 0;
        void operator()() { ++calls; }
    } counter;
    FunctionRef<void()> f{counter};
    f();
    f();
    CHECK(counter.calls == 2);
}

TEST_CASE("FunctionRef calls a function, by reference or by pointer")
{
    CHECK(sum(twice, {1, 2}) == 6);
    CHECK(sum(&twice, {1, 2}) == 6);
}

TEST_CASE("FunctionRef converts arguments and the result to its signature")
{
    FunctionRef<long(short)> widen{[](int x) { return x + 1; }};
    CHECK(widen(short{41}) == 42L);
    FunctionRef<void(std::string const&)> discard{[](std::string const& s) { return s.size(); }};
    discard("ignored");
}

TEST_CASE("FunctionRef passes a move-only argument through")
{
    struct MoveOnly
    {
        int v;
        MoveOnly(int v) : v{v} {}
        MoveOnly(MoveOnly&&) = default;
    };
    FunctionRef<int(MoveOnly)> f{[](MoveOnly m) { return m.v; }};
    CHECK(f(MoveOnly{7}) == 7);
}

TEST_CASE("FunctionRef moves a by-value argument only into the callable")
{
    struct Counted
    {
        int* moves;
        int* copies;
        Counted(int* moves, int* copies) : moves{moves}, copies{copies} {}
        Counted(Counted const& o) : moves{o.moves}, copies{o.copies} { ++*copies; }
        Counted(Counted&& o) noexcept : moves{o.moves}, copies{o.copies} { ++*moves; }
    };
    int moves = 0, copies = 0;
    FunctionRef<void(Counted)> f{[](Counted) {}};

    f(Counted{&moves, &copies});
    CHECK(moves == 1);
    CHECK(copies == 0);

    moves = 0;
    Counted lvalue{&moves, &copies};
    f(lvalue);
    CHECK(moves == 1);
    CHECK(copies == 1);
}

TEST_CASE("FunctionRef passes reference arguments through")
{
    FunctionRef<void(int&, int const&, int)> f{[](int& x, int const& y, int z) { x += y + z; }};
    int x = 1;
    f(x, 2, 3);
    CHECK(x == 6);
}

TEST_CASE("FunctionRef copied refers to the same callable")
{
    int calls = 0;
    auto bump = [&] { ++calls; };
    FunctionRef<void()> a{bump};
    FunctionRef<void()> b = a;
    a();
    b();
    CHECK(calls == 2);
}

TEST_CASE("FunctionRef refuses a callable not invocable with its signature")
{
    static_assert(not std::is_constructible_v<FunctionRef<void(int)>, decltype([](std::string const&) {})>);
    static_assert(std::is_constructible_v<FunctionRef<void(int)>, decltype([](long) {})>);
}

} // namespace arc::tests::function_ref
