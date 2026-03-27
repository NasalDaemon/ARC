module examples.calculator.functions;

import std;

namespace examples::calculator::node {

namespace {

struct FuncDef
{
    int arity;
    double(*fn)(std::span<double const>);
};

static const std::map<std::string, FuncDef, std::less<>> builtins = {
    {"abs",  {1, [](std::span<double const> a) { return std::abs(a[0]); }}},
    {"sqrt", {1, [](std::span<double const> a) { return std::sqrt(a[0]); }}},
    {"neg",  {1, [](std::span<double const> a) { return -a[0]; }}},
    {"sin",  {1, [](std::span<double const> a) { return std::sin(a[0]); }}},
    {"cos",  {1, [](std::span<double const> a) { return std::cos(a[0]); }}},
    {"tan",  {1, [](std::span<double const> a) { return std::tan(a[0]); }}},
    {"log",  {1, [](std::span<double const> a) { return std::log10(a[0]); }}},
    {"ln",   {1, [](std::span<double const> a) { return std::log(a[0]); }}},
    {"add",  {2, [](std::span<double const> a) { return a[0] + a[1]; }}},
    {"sub",  {2, [](std::span<double const> a) { return a[0] - a[1]; }}},
    {"mul",  {2, [](std::span<double const> a) { return a[0] * a[1]; }}},
    {"div",  {2, [](std::span<double const> a) { return a[0] / a[1]; }}},
    {"pow",  {2, [](std::span<double const> a) { return std::pow(a[0], a[1]); }}},
    {"min",  {2, [](std::span<double const> a) { return std::min(a[0], a[1]); }}},
    {"max",  {2, [](std::span<double const> a) { return std::max(a[0], a[1]); }}},
};

} // namespace

auto Functions::call(std::string_view name, std::span<double const> args) const
    -> std::expected<double, EvalError>
{
    auto it = builtins.find(name);
    if (it == builtins.end())
        return std::unexpected(EvalError{std::string("unknown function: ") + std::string(name)});

    const auto& def = it->second;
    if (static_cast<int>(args.size()) != def.arity)
        return std::unexpected(EvalError{
            std::string("wrong argument count for ") + std::string(name) +
            ": expected " + std::to_string(def.arity) +
            ", got " + std::to_string(args.size())
        });

    if (name == "div" && args[1] == 0.0)
        return std::unexpected(EvalError{"division by zero"});

    return def.fn(args);
}

auto Functions::list() const -> std::vector<std::string>
{
    std::vector<std::string> names;
    names.reserve(builtins.size());
    for (const auto& [name, _] : builtins)
        names.push_back(name);
    return names;
}

}
