#ifndef ARC_DOCTEST_DOCTEST_PRELUDE_H
#define ARC_DOCTEST_DOCTEST_PRELUDE_H

#include "arc/macros.hpp"

// Include at the top of test sources that include arc/doctest.h after imports.
// Without 'import std', GCC 16 rejects textual redeclarations of std entities
// after an import that brought them in through its global module fragment,
// so the std headers doctest needs must be seen before any import.
#define ARC_DOCTEST_PRELUDE 1

#if !ARC_IMPORT_STD && ARC_COMPILER_GE(GCC, 16)
#define ARC_DOCTEST_STD_HEADERS 1
#include <cstddef>
#include <ostream>
#include <istream>
#endif

#endif // ARC_DOCTEST_DOCTEST_PRELUDE_H
