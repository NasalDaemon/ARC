module examples.filesystem.filesystem:impl;

import examples.filesystem.filesystem;
import examples.filesystem.types;
import examples.filesystem.traits;
import std;

#define FILESYSTEM \
    template<class Context> \
    auto Filesystem::Node<Context>

namespace examples::filesystem::node {

FILESYSTEM::read(std::string_view path) const
    -> std::expected<DataView, FsError>
{
    auto normalised = getPathOps().normalise(path);

    auto entry = getStorage().get(normalised);
    if (!entry)
        return std::unexpected(FsError::NotFound);

    if (entry->isDir())
        return std::unexpected(FsError::IsADirectory);

    return entry->content();
}

FILESYSTEM::write(std::string_view path, std::string data)
    -> std::expected<void, FsError>
{
    auto pathOps = getPathOps();
    auto storage = getStorage();

    std::string normalised = pathOps.normalise(path);

    if (pathOps.isRoot(normalised))
        return std::unexpected(FsError::IsADirectory);

    // Check parent exists and is a directory
    std::string parentPath = pathOps.parent(normalised);
    auto parentEntry = storage.get(parentPath);
    if (!parentEntry)
        return std::unexpected(FsError::NotFound);
    if (!parentEntry->isDir())
        return std::unexpected(FsError::NotADirectory);

    // Check if target exists and is not a directory
    if (auto existing = storage.get(normalised); existing && existing->isDir())
        return std::unexpected(FsError::IsADirectory);

    return storage.put(normalised, Entry::file(std::move(data)));
}

FILESYSTEM::mkdir(std::string_view path)
    -> std::expected<void, FsError>
{
    auto pathOps = getPathOps();
    auto storage = getStorage();

    std::string normalised = pathOps.normalise(path);

    if (pathOps.isRoot(normalised))
        return std::unexpected(FsError::AlreadyExists);

    // Check if already exists
    if (storage.get(normalised))
        return std::unexpected(FsError::AlreadyExists);

    // Check parent exists and is a directory
    std::string parentPath = pathOps.parent(normalised);
    auto parentEntry = storage.get(parentPath);
    if (!parentEntry)
        return std::unexpected(FsError::NotFound);
    if (!parentEntry->isDir())
        return std::unexpected(FsError::NotADirectory);

    storage.put(normalised, Entry::directory());
    return {};
}

FILESYSTEM::remove(std::string_view path)
    -> std::expected<void, FsError>
{
    auto pathOps = getPathOps();
    auto storage = getStorage();

    std::string normalised = pathOps.normalise(path);

    if (pathOps.isRoot(normalised))
        return std::unexpected(FsError::InvalidPath);

    auto entry = storage.get(normalised);
    if (!entry)
        return std::unexpected(FsError::NotFound);

    // If directory, check if empty
    if (entry->isDir())
    {
        auto children = storage.children(normalised);
        if (!children.empty())
            return std::unexpected(FsError::NotEmpty);
    }

    storage.erase(normalised);
    return {};
}

FILESYSTEM::list(std::string_view path) const
    -> std::expected<Children, FsError>
{
    std::string normalised = getPathOps().normalise(path);

    auto storage = getStorage();
    auto entry = storage.get(normalised);
    if (!entry)
        return std::unexpected(FsError::NotFound);

    if (!entry->isDir())
        return std::unexpected(FsError::NotADirectory);

    return storage.children(normalised);
}

FILESYSTEM::exists(std::string_view path) const -> bool
{
    auto normalised = getPathOps().normalise(path);
    return static_cast<bool>(getStorage().get(normalised));
}

FILESYSTEM::isDir(std::string_view path) const -> bool
{
    auto normalised = getPathOps().normalise(path);
    auto entry = getStorage().get(normalised);
    return entry && entry->isDir();
}

} // namespace examples::filesystem::node
