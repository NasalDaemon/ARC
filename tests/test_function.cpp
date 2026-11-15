#include "arc/doctest_prelude.h"
#include "arc/macros.hpp"

#if !ARC_IMPORT_STD
#include <memory>
#include <string>
#endif

import arc;

#include "arc/doctest.h"

namespace arc::tests::function {

namespace {

struct Counted
{
    int* moves;
    int* copies;
    Counted(int* moves, int* copies) : moves{moves}, copies{copies} {}
    Counted(Counted const& o) : moves{o.moves}, copies{o.copies} { ++*copies; }
    Counted(Counted&& o) noexcept : moves{o.moves}, copies{o.copies} { ++*moves; }
};

} // namespace

TEST_CASE("Function moves a by-value argument only into the callable")
{
    int moves = 0, copies = 0;
    Function<void(Counted)> f{[](Counted) {}};

    f(Counted{&moves, &copies});
    CHECK(moves == 1);
    CHECK(copies == 0);

    moves = 0;
    Counted lvalue{&moves, &copies};
    f(lvalue);
    CHECK(moves == 1);
    CHECK(copies == 1);
}

TEST_CASE("Function const call moves a by-value argument only into the callable")
{
    int moves = 0, copies = 0;
    Function<void(Counted), FunctionPolicy{.copyable=false, .mutableCall=false, .constCall=true}> const f{[](Counted) {}};
    f(Counted{&moves, &copies});
    CHECK(moves == 1);
    CHECK(copies == 0);
}

TEST_CASE("Function passes reference and move-only arguments through")
{
    Function<void(int&, int const&, int)> add{[](int& x, int const& y, int z) { x += y + z; }};
    int x = 1;
    add(x, 2, 3);
    CHECK(x == 6);

    Function<int(std::unique_ptr<int>)> deref{[](std::unique_ptr<int> p) { return *p; }};
    CHECK(deref(std::make_unique<int>(7)) == 7);
}

TEST_CASE("Function converts arguments and the result to its signature")
{
    Function<long(short)> widen{[](int x) { return x + 1; }};
    CHECK(widen(short{41}) == 42L);
    Function<void(std::string const&)> discard{[](std::string const& s) { return s.size(); }};
    discard("ignored");
}

TEST_CASE("Function calls a stateful callable, keeping its state")
{
    int calls = 0;
    Function<int()> f{[n = 0, &calls]() mutable { ++calls; return ++n; }};
    CHECK(f() == 1);
    CHECK(f() == 2);
    CHECK(calls == 2);
}

} // namespace arc::tests::function
