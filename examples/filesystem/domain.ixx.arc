export module examples.filesystem.domain;

import examples.filesystem.filesystem;
import examples.filesystem.memory_storage;
import examples.filesystem.path_ops;
import examples.filesystem.traits;

namespace examples::filesystem {

domain FilesystemDomain [Root]
{
    fs = Filesystem
    pathOps = PathOps
    Storage = Root::Storage

    [trait::Filesystem] .. --> fs
    [trait::PathOps]           fs --> pathOps
    [trait::Storage]           fs --> Storage
}

}
