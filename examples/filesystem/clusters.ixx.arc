export module examples.filesystem.clusters;

import examples.filesystem.filesystem;
import examples.filesystem.memory_storage;
import examples.filesystem.path_ops;
import examples.filesystem.repl;
import examples.filesystem.traits;

namespace examples::filesystem::cluster {

cluster Filesystem [Root]
{
    fs = filesystem::Filesystem
    pathOps = PathOps
    storage = Root::FilesystemStorage

    [trait::Filesystem] .. --> fs
    [trait::PathOps]           fs --> pathOps
    [trait::Storage]           fs --> storage

    [trait::DirectorySync]
    .. --> storage
}

cluster Repl
{
    repl = filesystem::Repl
    fs = Filesystem

    [trait::Filesystem]    repl --> fs
    [trait::DirectorySync] repl --> fs
}

}
