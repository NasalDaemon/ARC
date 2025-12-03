module examples.filesystem.disk_storage;

namespace examples::filesystem {

DiskStorage::DiskStorage()
    : rootPath{"."}
{}

auto DiskStorage::toAbsolute(std::string_view path) const -> std::filesystem::path
{
    // path is expected to be normalised starting with "/"
    // We strip the leading "/" and append to rootPath
    if (path.empty() || path == "/")
        return rootPath;

    if (path.starts_with('/'))
        path.remove_prefix(1);

    return rootPath / path;
}

auto DiskStorage::impl(trait::Storage::get, std::string_view path) const -> std::optional<DiskEntry>
{
    auto const absPath = toAbsolute(path);

    std::error_code ec;
    if (!std::filesystem::exists(absPath, ec) || ec)
        return std::nullopt;

    if (std::filesystem::is_directory(absPath, ec))
        return DiskEntry::directory(absPath);
    else
        return DiskEntry::file(absPath);
}

auto DiskStorage::impl(trait::Storage::put, std::string_view path, InMemoryEntry entry) -> std::expected<void, FsError>
{
    auto const absPath = toAbsolute(path);

    if (entry.isDir())
    {
        std::error_code ec;
        std::filesystem::create_directories(absPath, ec);
        if (ec)
            return std::unexpected(FsError::InvalidPath);
    }
    else
    {
        // Ensure parent directory exists
        if (auto parent = absPath.parent_path(); !parent.empty())
        {
            std::error_code ec;
            std::filesystem::create_directories(parent, ec);
            if (ec)
                return std::unexpected(FsError::InvalidPath);
        }

        std::ofstream file{absPath, std::ios::binary | std::ios::trunc};
        if (!file)
            return std::unexpected(FsError::InvalidPath);
        file << entry.content();
    }
    return {};
}

auto DiskStorage::impl(trait::Storage::erase, std::string_view path) -> bool
{
    auto const absPath = toAbsolute(path);

    std::error_code ec;
    if (!std::filesystem::exists(absPath, ec) || ec)
        return false;

    return std::filesystem::remove(absPath, ec) && !ec;
}

auto DiskStorage::impl(trait::Storage::children, std::string_view path) const -> std::vector<std::string>
{
    auto const absPath = toAbsolute(path);

    std::vector<std::string> result;

    std::error_code ec;
    if (!std::filesystem::is_directory(absPath, ec) || ec)
        return result;

    for (auto const& entry : std::filesystem::directory_iterator(absPath, ec))
    {
        if (ec)
            break;
        result.push_back(entry.path().filename().string());
    }

    return result;
}

auto DiskStorage::impl(trait::DirectorySync::loadFromDirectory, std::string_view directory) -> std::expected<void, FsError>
{
    std::error_code ec;
    auto const dirPath = std::filesystem::path{directory};

    if (!std::filesystem::exists(dirPath, ec) || ec)
        return std::unexpected{FsError::NotFound};

    if (!std::filesystem::is_directory(dirPath, ec) || ec)
        return std::unexpected{FsError::NotADirectory};

    rootPath = std::filesystem::canonical(dirPath, ec);
    if (ec)
        return std::unexpected{FsError::InvalidPath};

    return {};
}

auto DiskStorage::impl(trait::DirectorySync::dumpToDirectory, std::string_view directory) const -> std::expected<void, FsError>
{
    // For DiskStorage, if directory matches rootPath, it's a noop
    // Otherwise, copy the entire directory tree
    std::error_code ec;
    auto const destPath = std::filesystem::path{directory};

    if (std::filesystem::equivalent(rootPath, destPath, ec) && !ec)
        return {}; // Already at this location

    // Copy entire tree to new location
    std::filesystem::copy(rootPath, destPath,
        std::filesystem::copy_options::recursive | std::filesystem::copy_options::overwrite_existing, ec);

    if (ec)
        return std::unexpected{FsError::InvalidPath};

    return {};
}

}
