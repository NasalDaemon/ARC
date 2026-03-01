module;
#if !ARC_IMPORT_STD
#include <cstdio>
#endif
module abc.ellie;

#if ARC_IMPORT_STD
import std;
#endif

void abc::node::Ellie::onGraphConstructed() { std::puts("Constructed Ellie"); }

int abc::node::Ellie::impl(trait::Ellie::get) const { return value; }
