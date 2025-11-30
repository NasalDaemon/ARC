module examples.filesystem.memory_storage;

import std;

namespace examples::filesystem {

MemoryStorage::MemoryStorage()
{
    // Initialize with root directory
    entries["/"] = Entry::directory();
}

auto MemoryStorage::impl(trait::Storage::get, std::string_view path) const -> Entry const*
{
    auto it = entries.find(path);
    if (it == entries.end())
        return nullptr;
    return &it->second;
}

auto MemoryStorage::impl(trait::Storage::put, std::string_view path, Entry entry) -> void
{
    auto const it = entries.insert_or_assign(std::string(path), std::move(entry)).first;
    auto parentDir = parent(path);
    auto& c = children.try_emplace(std::string(parentDir)).first->second;
    c.push_back(std::to_address(it));
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

auto MemoryStorage::impl(trait::Storage::erase, std::string_view path) -> bool
{
    auto parentDir = parent(path);
    if (auto const it = children.find(parentDir); it != children.end())
        std::erase_if(it->second, [path](auto const* pair) { return pair->first == path; });

    return entries.erase(std::string(path)) > 0;
}

auto MemoryStorage::impl(trait::Storage::children, std::string_view path) const -> std::vector<std::string_view>
{
    std::vector<std::string_view> result;
    auto it = children.find(path);
    if (it == children.end())
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

}
