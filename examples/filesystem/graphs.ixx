export module examples.filesystem.graphs;

import examples.filesystem.clusters;
import examples.filesystem.memory_storage;
import examples.filesystem.disk_storage;

import arc;

namespace examples::filesystem::graph {

struct InMemoryRoot
{
    using FilesystemStorage = MemoryStorage;
};
export using InMemory = arc::Graph<cluster::Filesystem, InMemoryRoot>;
export using InMemoryRepl = arc::Graph<cluster::Repl, InMemoryRoot>;

struct DiskRoot
{
    using FilesystemStorage = DiskStorage;
};
export using Disk = arc::Graph<cluster::Filesystem, DiskRoot>;
export using DiskRepl = arc::Graph<cluster::Repl, DiskRoot>;

}
