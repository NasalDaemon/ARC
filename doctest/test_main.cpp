#define DOCTEST_CONFIG_IMPLEMENT
#include "doctest/doctest.h"

#include <cstdio>

int main(int argc, char** argv) {
    // A test case registers itself from a static initialiser in its own object,
    // which nothing references. Linked from an archive, the linker drops that
    // object and the binary would pass having run nothing. Checked on what is
    // registered, not what runs, so a filter matching nothing still passes.
    if (doctest::detail::getRegisteredTests().empty()) {
        std::fputs("doctest: no test cases are registered in this binary, so the "
                   "objects defining them were not linked. Link the test sources' "
                   "objects directly, or their static library whole (whole-archive), "
                   "not from a static library the linker takes only referenced "
                   "objects from.\n", stderr);
        return 1;
    }
    doctest::Context context(argc, argv);
    return context.run();
}
