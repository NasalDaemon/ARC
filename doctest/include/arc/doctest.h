#ifndef ARC_DOCTEST_DOCTEST_H
#define ARC_DOCTEST_DOCTEST_H

#if ARC_IMPORT_STD
// Prevent doctest from including std library headers
#define _GLIBCXX_CISO646
#define _LIBCPP_CSTDDEF
#define _LIBCPP_OSTREAM
#define _LIBCPP_ISTREAM
#define _LIBCPP_TYPE_TRAITS
#endif

#include "doctest/doctest.h"

#endif // ARC_TESTS_DOCTEST_H
