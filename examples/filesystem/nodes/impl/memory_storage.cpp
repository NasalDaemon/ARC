module examples.filesystem.memory_storage;

import std;

namespace examples::filesystem::node {

MemoryStorage::MemoryStorage()
{
    // Initialize with root directory
    entries.try_emplace("/", Entry::directory());
}

auto MemoryStorage::get(std::string_view path) const -> Entry const*
{
    auto it = entries.find(path);
    if (it == entries.end())
        return nullptr;
    return &it->second;
}

auto MemoryStorage::put(std::string_view path, Entry entry) -> std::expected<void, FsError>
{
    auto const [it, inserted] = entries.try_emplace(std::string(path), std::move(entry));
    if (inserted)
    {
        auto parentDir = parent(path);
        auto& c = childrenMap.try_emplace(std::string(parentDir)).first->second;
        c.push_back(std::to_address(it));
    }
    else
    {
        if (it->second.isDir())
        {
            // Directory can only be overwritten by another directory (no-op)
            if (not entry.isDir()) [[unlikely]]
            {
                return std::unexpected(FsError::IsADirectory);
            }
        }
        else
        {
            it->second = std::move(entry);
        }
    }
    return {};
}

auto MemoryStorage::parent(std::string_view path) -> std::string_view
{
    auto slashPos = path.rfind('/');
    if (slashPos == std::string_view::npos)
        return "/";
    if (slashPos == 0)
        return "/";
    return path.substr(0, slashPos);
}

auto MemoryStorage::erase(std::string_view path) -> bool
{
    auto parentDir = parent(path);
    if (auto const it = childrenMap.find(parentDir); it != childrenMap.end())
        std::erase_if(it->second, [path](auto const* pair) { return pair->first == path; });

    return entries.erase(std::string(path)) > 0;
}

auto MemoryStorage::children(std::string_view path) const -> std::vector<std::string_view>
{
    std::vector<std::string_view> result;
    auto it = childrenMap.find(path);
    if (it == childrenMap.end())
        return result;
    auto const prefix = path.size() + (path != "/");
    for (auto const* pair : it->second)
    {
        std::string_view child = pair->first;
        result.emplace_back(child.substr(prefix));
    }

    std::ranges::sort(result);
    return result;
}

} // namespace examples::filesystem::node
