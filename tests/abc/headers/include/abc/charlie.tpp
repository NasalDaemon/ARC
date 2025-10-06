#pragma once

#include "abc/charlie.hpp"

#include "arc/test.hpp"

#define CHARLIE \
    template<class Context> \
    auto abc::Charlie::Node<Context>

CHARLIE::Alice::impl(trait::Alice::get) const -> int
{
    static_assert(arc::CanGetNode<Node, trait::AliceRead>);
    auto const value = getNode(trait::aliceRead).get();

    if constexpr (arc::test::IsTestContext<Context>)
        return value + 10;
    else
        return value;
};

CHARLIE::Charlie::impl(trait::Charlie::get) const -> int
{
     return charlie;
}


CHARLIE::impl(trait::Visitable::count, int& counter) -> void
{
    std::puts("trait::Visitable::count: Visited Charlie");
    counter++;
}
