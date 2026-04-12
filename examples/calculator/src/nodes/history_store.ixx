export module examples.calculator.node.history_store;

import examples.calculator.traits;
import arc;
import std;

namespace examples::calculator::node {

export struct HistoryStore : arc::NodeImpl<History>
{
    auto addEntry(std::string line) -> void;
    auto entries() const -> std::vector<std::string>;

private:
    std::vector<std::string> entries_;
};

} // namespace examples::calculator::node
