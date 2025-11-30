export module examples.filesystem.graphs;

import examples.filesystem.domain;
import examples.filesystem.memory_storage;

import arc;

namespace examples::filesystem {

export struct InMemoryRoot
{
    using Storage = MemoryStorage;
};
export using InMemoryGraph = arc::Graph<FilesystemDomain, InMemoryRoot>;

// Implement your own disk-based storage
// export struct HarddiskRoot
// {
//     using Storage = DiskStorage;
// };
// export using HarddiskGraph = arc::Graph<FilesystemDomain, HarddiskRoot>;

}
