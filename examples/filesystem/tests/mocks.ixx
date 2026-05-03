export module examples.filesystem.tests.mocks;

import examples.filesystem.types;
import examples.filesystem.traits;
import std;

namespace examples::filesystem::tests {

// Types for the mock Filesystem trait
export struct MockFilesystemTypes
{
    using DataView = std::string_view;
    using Children = std::vector<std::string_view>;
};

// Types for the mock Storage trait
export struct MockStorageTypes
{
    using GetResult = Entry const*;
    using Children = std::vector<std::string_view>;
};

// Mock storage state for testing
export struct MockStorage
{
    std::map<std::string, Entry, std::less<>> entries;
    std::map<std::string, std::vector<std::string_view>, std::less<>> childrenMap;

    explicit MockStorage(auto& graph)
    {
        entries.try_emplace("/", Entry::directory());
        graph.mocks->define(
            [this](trait::Storage::get, std::string_view path) {
                return get(path);
            },
            [this](trait::Storage::put, std::string_view path, Entry entry) {
                return put(path, std::move(entry));
            },
            [this](trait::Storage::erase, std::string_view path) {
                return erase(path);
            },
            [this](trait::Storage::children, std::string_view path) {
                return children(path);
            }
        );
    }

    auto get(std::string_view path) const -> Entry const*
    {
        auto it = entries.find(path);
        return it != entries.end() ? &it->second : nullptr;
    }

    auto put(std::string_view path, Entry entry) -> std::expected<void, FsError>
    {
        entries.insert_or_assign(std::string(path), std::move(entry));
        return {};
    }

    auto erase(std::string_view path) -> bool
    {
        return entries.erase(std::string(path)) > 0;
    }

    auto children(std::string_view path) -> std::vector<std::string_view>
    {
        auto it = childrenMap.find(path);
        return it != childrenMap.end() ? it->second : std::vector<std::string_view>{};
    }
};

} // namespace examples::filesystem::tests
