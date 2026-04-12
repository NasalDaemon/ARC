module examples.calculator.node.history_store;

import std;

namespace examples::calculator::node {

auto HistoryStore::addEntry(std::string line) -> void
{
    entries_.emplace_back(std::move(line));
}

auto HistoryStore::entries() const -> std::vector<std::string>
{
    return entries_;
}

} // namespace examples::calculator::node
