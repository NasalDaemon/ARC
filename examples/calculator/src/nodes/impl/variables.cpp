module examples.calculator.variables;

import std;

namespace examples::calculator::node {

auto Variables::get(std::string_view name) const -> std::optional<double>
{
    auto it = vars_.find(name);
    if (it == vars_.end())
        return std::nullopt;
    return it->second;
}

void Variables::set(std::string name, double value)
{
    vars_[std::move(name)] = value;
}

auto Variables::remove(std::string_view name) -> bool
{
    return vars_.erase(std::string(name)) > 0;
}

auto Variables::list() const -> std::vector<std::pair<std::string, double>>
{
    return {vars_.begin(), vars_.end()};
}

void Variables::clear()
{
    vars_.clear();
}

} // namespace examples::calculator::node
