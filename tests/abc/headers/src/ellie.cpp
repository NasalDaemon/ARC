#include "abc/ellie.hpp"

void abc::node::Ellie::onGraphConstructed() { std::puts("Constructed Ellie"); }

int abc::node::Ellie::impl(trait::Ellie::get) const { return value; }
