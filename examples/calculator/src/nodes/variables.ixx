export module examples.calculator.node.variables;

import examples.calculator.traits;
import arc;
import std;

namespace examples::calculator::node {

export struct Variables : arc::NodeImpl<trait::Variables>
{
    auto get(std::string_view name) const -> std::optional<double>;
    void set(std::string name, double value);
    auto remove(std::string_view name) -> bool;
    auto list() const -> std::vector<std::pair<std::string, double>>;
    void clear();

private:
    std::map<std::string, double, std::less<>> vars_;
};

} // namespace examples::calculator::node
