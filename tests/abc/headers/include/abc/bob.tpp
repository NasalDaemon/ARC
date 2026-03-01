#pragma once

#include "abc/bob.hpp"

#define BOB \
    template<class Context> \
    auto abc::node::Bob::Node<Context>

BOB::impl(trait::Alice::get) const -> int
{
    return getNode(trait::alice).get();
}

BOB::impl(trait::Bob::get) const -> int
{
    return bob;
}

BOB::impl(trait::Bob::set, int value) -> void
{
    bob = value;
}

BOB::impl(trait::Charlie::get) const -> int
{
    return getNode(trait::charlie).get();
}
