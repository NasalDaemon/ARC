module;
#if !ARC_IMPORT_STD
#include <cstdio>
#endif
module abc.ellie;

#if ARC_IMPORT_STD
import std;
#endif

void abc::Ellie::onGraphConstructed() { std::puts("Constructed Ellie"); }

int abc::Ellie::impl(trait::Ellie::get) const { return value; }
