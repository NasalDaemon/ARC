export module examples.filesystem.tests.mocks;

import examples.filesystem.entry;
import std;

namespace examples::filesystem::tests {

// Types for the mock Storage trait
export struct MockStorageTypes
{
    using GetResult = InMemoryEntry const*;
    using Children = std::vector<std::string_view>;
};

// Mock storage state for testing
export struct MockStorage
{
    std::map<std::string, InMemoryEntry, std::less<>> entries;
    std::map<std::string, std::vector<std::string_view>, std::less<>> childrenMap;

    MockStorage()
    {
        entries.try_emplace("/", InMemoryEntry::directory());
    }

    auto get(std::string_view path) const -> InMemoryEntry const*
    {
        auto it = entries.find(path);
        return it != entries.end() ? &it->second : nullptr;
    }

    auto put(std::string_view path, InMemoryEntry entry) -> std::expected<void, FsError>
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
