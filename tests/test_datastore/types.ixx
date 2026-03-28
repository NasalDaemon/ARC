module;
#include "arc/macros.hpp"

#if !ARC_IMPORT_STD
#include <cstddef>
#include <string>
#endif

export module arc.tests.datastore.types;

#if ARC_IMPORT_STD
import std;
#endif

namespace arc::tests::datastore {

export struct SharedData
{
    using Id = int;
    explicit SharedData(Id id) : id(id) {}

    Id id;
    std::string extra{};
};

} // namespace arc::tests::datastore
