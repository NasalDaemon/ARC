export module examples.filesystem.entry;

import std;

namespace examples::filesystem {

// Filesystem error types
export enum class FsError
{
    NotFound,
    NotADirectory,
    IsADirectory,
    AlreadyExists,
    InvalidPath,
};

export constexpr std::string_view asString(FsError error)
{
    switch (error)
    {
        case FsError::NotFound: return "not found";
        case FsError::NotADirectory: return "not a directory";
        case FsError::IsADirectory: return "is a directory";
        case FsError::AlreadyExists: return "already exists";
        case FsError::InvalidPath: return "invalid path";
    }
    return "unknown error";
}

// Enable streaming for FsError (used by doctest for stringification)
export inline auto operator<<(std::ostream& os, FsError error) -> std::ostream&
{
    return os << asString(error);
}

// Concept for filesystem entries (covers both InMemoryEntry and DiskEntry)
export template<class T>
concept Entry = requires(T const& entry) {
    typename T::DataView;
    { entry.isDir() } -> std::same_as<bool>;
    { entry.stream() } -> std::convertible_to<std::istream>;
    { entry.content() } -> std::convertible_to<typename T::DataView>;
};

// Represents an in-memory filesystem entry (file or directory)
export struct InMemoryEntry
{
    using DataView = std::string_view;

    auto isDir() const -> bool { return isDir_; }

    auto stream() const -> std::istringstream
    {
        return std::istringstream{data_};
    }

    auto content() const -> DataView
    {
        return data_;
    }

    static auto file(std::string content = {}) -> InMemoryEntry
    {
        return InMemoryEntry{false, std::move(content)};
    }

    static auto directory() -> InMemoryEntry
    {
        return InMemoryEntry{true, {}};
    }

private:
    bool isDir_ = false;
    std::string data_; // Empty for directories

    InMemoryEntry(bool isDir, std::string data)
        : isDir_{isDir}, data_{std::move(data)}
    {}
};

// Represents a disk filesystem entry (metadata + lazy file reading)
export struct DiskEntry
{
    using DataView = std::string;

    auto isDir() const -> bool { return isDir_; }

    auto stream() const -> std::ifstream
    {
        return std::ifstream{path_, std::ios::binary};
    }

    auto content() const -> std::string
    {
        if (isDir_)
            return {};
        std::ifstream file{path_, std::ios::binary};
        std::ostringstream ss;
        ss << file.rdbuf();
        return std::move(ss).str();
    }

    static auto file(std::filesystem::path p) -> DiskEntry
    {
        return DiskEntry{false, std::move(p)};
    }

    static auto directory(std::filesystem::path p) -> DiskEntry
    {
        return DiskEntry{true, std::move(p)};
    }

private:
    bool isDir_ = false;
    std::filesystem::path path_;

    DiskEntry(bool isDir, std::filesystem::path path)
        : isDir_{isDir}, path_{std::move(path)}
    {}
};

}
