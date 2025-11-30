module examples.filesystem.filesystem:impl;

import examples.filesystem.filesystem;
import examples.filesystem.entry;
import examples.filesystem.traits;
import std;

#define FILESYSTEM \
    template<class Context> \
    auto Filesystem::Node<Context>

namespace examples::filesystem {

FILESYSTEM::impl(trait::Filesystem::read, std::string_view path) const
    -> std::expected<DataView, FsError>
{
    std::string normalised = getNode(trait::pathOps).normalise(path);

    auto entry = getNode(trait::storage).get(normalised);
    if (!entry)
        return std::unexpected(FsError::NotFound);

    if (entry->isDir)
        return std::unexpected(FsError::IsADirectory);

    return entry->content;
}

FILESYSTEM::impl(trait::Filesystem::write, std::string_view path, std::string data)
    -> std::expected<void, FsError>
{
    auto pathOps = getNode(trait::pathOps);
    auto storage = getNode(trait::storage);

    std::string normalised = pathOps.normalise(path);

    if (pathOps.isRoot(normalised))
        return std::unexpected(FsError::IsADirectory);

    // Check parent exists and is a directory
    std::string parentPath = pathOps.parent(normalised);
    auto parentEntry = storage.get(parentPath);
    if (!parentEntry)
        return std::unexpected(FsError::NotFound);
    if (!parentEntry->isDir)
        return std::unexpected(FsError::NotADirectory);

    // Check if target exists and is not a directory
    auto existing = storage.get(normalised);
    if (existing && existing->isDir)
        return std::unexpected(FsError::IsADirectory);

    storage.put(normalised, Entry::file(std::move(data)));
    return {};
}

FILESYSTEM::impl(trait::Filesystem::mkdir, std::string_view path)
    -> std::expected<void, FsError>
{
    auto pathOps = getNode(trait::pathOps);
    auto storage = getNode(trait::storage);

    std::string normalised = pathOps.normalise(path);

    if (pathOps.isRoot(normalised))
        return std::unexpected(FsError::AlreadyExists);

    // Check if already exists
    auto existing = storage.get(normalised);
    if (existing)
        return std::unexpected(FsError::AlreadyExists);

    // Check parent exists and is a directory
    std::string parentPath = pathOps.parent(normalised);
    auto parentEntry = storage.get(parentPath);
    if (!parentEntry)
        return std::unexpected(FsError::NotFound);
    if (!parentEntry->isDir)
        return std::unexpected(FsError::NotADirectory);

    storage.put(normalised, Entry::directory());
    return {};
}

FILESYSTEM::impl(trait::Filesystem::remove, std::string_view path)
    -> std::expected<void, FsError>
{
    auto pathOps = getNode(trait::pathOps);
    auto storage = getNode(trait::storage);

    std::string normalised = pathOps.normalise(path);

    if (pathOps.isRoot(normalised))
        return std::unexpected(FsError::InvalidPath);

    auto entry = storage.get(normalised);
    if (!entry)
        return std::unexpected(FsError::NotFound);

    // If directory, check if empty
    if (entry->isDir)
    {
        auto children = storage.children(normalised);
        if (!children.empty())
            return std::unexpected(FsError::InvalidPath); // Directory not empty
    }

    storage.erase(normalised);
    return {};
}

FILESYSTEM::impl(trait::Filesystem::list, std::string_view path) const
    -> std::expected<std::vector<std::string_view>, FsError>
{
    auto pathOps = getNode(trait::pathOps);
    auto storage = getNode(trait::storage);

    std::string normalised = pathOps.normalise(path);

    auto entry = storage.get(normalised);
    if (!entry)
        return std::unexpected(FsError::NotFound);

    if (!entry->isDir)
        return std::unexpected(FsError::NotADirectory);

    return storage.children(normalised);
}

FILESYSTEM::impl(trait::Filesystem::exists, std::string_view path) const -> bool
{
    auto pathOps = getNode(trait::pathOps);
    auto storage = getNode(trait::storage);

    std::string normalised = pathOps.normalise(path);
    return storage.get(normalised) != nullptr;
}

FILESYSTEM::impl(trait::Filesystem::isDir, std::string_view path) const -> bool
{
    auto pathOps = getNode(trait::pathOps);
    auto storage = getNode(trait::storage);

    std::string normalised = pathOps.normalise(path);
    auto entry = storage.get(normalised);
    return entry && entry->isDir;
}

}
