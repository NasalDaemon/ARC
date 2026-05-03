module examples.filesystem.memory_storage:load;
import examples.filesystem.memory_storage;

import examples.filesystem.types;
import examples.filesystem.traits;

import std;

namespace examples::filesystem::node {

auto MemoryStorage::loadFromDirectory(std::string_view dirPath) -> std::expected<void, FsError>
{
    try
    {
        std::filesystem::path rootPath{dirPath};

        if (not std::filesystem::exists(rootPath) || not std::filesystem::is_directory(rootPath))
            return std::unexpected(FsError::InvalidPath);

        auto const loadRecursive =
            // Weird GCC bug requires 'this' to be captured as a named value
            [p = this](this auto self, std::filesystem::path const& currentPath, std::string fsPath) -> void
            {
                for (auto const& entry : std::filesystem::directory_iterator{currentPath})
                {
                    std::string name = entry.path().filename().string();
                    std::string fullFsPath = std::format("{}/{}", fsPath, name);

                    if (entry.is_directory())
                    {
                        if (auto result = p->put(fullFsPath, Entry::directory()); result)
                            self(entry.path(), fullFsPath);
                    }
                    else if (entry.is_regular_file())
                    {
                        try
                        {
                            std::ifstream file(entry.path());
                            std::string content(std::istreambuf_iterator<char>{file}, {});
                            p->put(fullFsPath, Entry::file(std::move(content)));
                        }
                        catch (std::exception const&)
                        {
                            // Skip files that cannot be read
                        }
                    }
                }
            };

        loadRecursive(rootPath, "");
        return {};
    }
    catch (std::exception const&)
    {
        return std::unexpected(FsError::InvalidPath);
    }
}

auto MemoryStorage::dumpToDirectory(std::string_view dirPath) const -> std::expected<void, FsError>
{
    try
    {
        std::filesystem::path rootPath{dirPath};

        // Create the root directory if it doesn't exist
        if (not std::filesystem::exists(rootPath))
            std::filesystem::create_directories(rootPath);

        for (auto const& [path, entry] : entries)
        {
            // Skip root
            if (path == "/")
                continue;

            std::filesystem::path fullPath = rootPath / path.substr(1); // Remove leading /

            if (entry.isDir())
            {
                std::filesystem::create_directories(fullPath);
            }
            else
            {
                // Ensure parent directory exists
                std::filesystem::create_directories(fullPath.parent_path());

                std::ofstream file(fullPath);
                if (not file)
                    return std::unexpected(FsError::InvalidPath);
                file << entry.content();
            }
        }

        return {};
    }
    catch (std::exception const&)
    {
        return std::unexpected(FsError::InvalidPath);
    }
}

} // namespace examples::filesystem::node
