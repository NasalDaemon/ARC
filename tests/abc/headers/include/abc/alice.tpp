#pragma once

#include "abc/alice.hpp"

#define ALICE \
    template<class Context> \
    auto abc::node::Alice::Node<Context>

ALICE::impl(trait::Bob::get) const -> int
{
    return getNode(trait::bob).get();
}
ALICE::impl(trait::Bob::set, int) -> void
{
}

ALICE::impl(trait::Charlie::get method) const -> int
{
    return getNode(traitOf(method)).get();
}
