module examples.calculator.history_store;

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

}
