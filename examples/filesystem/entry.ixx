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
export inline std::ostream& operator<<(std::ostream& os, FsError error)
{
    return os << asString(error);
}

// Represents a filesystem entry (file or directory)
export struct Entry
{
    using DataView = std::string_view;

    bool isDir = false;
    std::string content; // Empty for directories

    static Entry file(std::string content = {})
    {
        return Entry{.isDir = false, .content = std::move(content)};
    }

    static Entry directory()
    {
        return Entry{.isDir = true, .content = {}};
    }
};

}
