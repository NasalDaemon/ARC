module examples.calculator.node.functions:impl;

import examples.calculator.node.functions;
import examples.calculator.types;
import examples.calculator.traits;
import arc;
import std;

#define FUNCTIONS \
    template<class Context> \
    auto Functions::Node<Context>

namespace examples::calculator::node {

struct FuncDef
{
    int arity;
    double(*fn)(std::span<double const>);
};

std::map<std::string, FuncDef, std::less<>> const builtins = {
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

using DepMap = std::map<Functions::FuncKey, std::set<Functions::FuncKey>, std::less<>>;

auto hasCycle(Functions::FuncKey const& targetKey, DepMap const& deps) -> bool
{
    std::set<Functions::FuncKey, std::less<>> visited;
    std::set<Functions::FuncKey, std::less<>> recStack;

    auto const dfs = [&](this auto const& self, Functions::FuncKey const& key) -> bool {
        if (recStack.count(key) > 0)
            return true;
        if (visited.count(key) > 0)
            return false;

        visited.insert(key);
        recStack.insert(key);

        auto it = deps.find(key);
        if (it != deps.end())
        {
            for (auto const& dep : it->second)
            {
                if (self(dep))
                    return true;
            }
        }

        recStack.erase(key);
        return false;
    };

    return dfs(targetKey);
}

FUNCTIONS::call(std::string_view name, std::span<double const> args) const
    -> std::expected<double, EvalError>
{
    auto it = builtins.find(name);
    if (it == builtins.end() || static_cast<int>(args.size()) != it->second.arity)
        return std::unexpected(EvalError{std::format("unknown function: {}/{}", name, args.size())});

    const auto& def = it->second;
    if (name == "div" && args[1] == 0.0)
        return std::unexpected(EvalError{"division by zero"});

    return def.fn(args);
}

FUNCTIONS::impl(BuiltinFunctions::list) const -> std::vector<std::string>
{
    std::vector<std::string> names;
    names.reserve(builtins.size());
    for (const auto& [name, _] : builtins)
        names.push_back(name);
    return names;
}

FUNCTIONS::define(std::string name, std::vector<std::string> params, ExprPtr body, std::string source)
    -> std::expected<void, EvalError>
{
    FunctionDefinition def{std::move(name), std::move(params), std::move(body), std::move(source)};
    return define(std::span{&def, 1});
}

FUNCTIONS::define(std::span<FunctionDefinition const> defs) -> std::expected<void, EvalError>
{
    std::vector<std::set<FuncKey>> allDeps(defs.size());
    for (std::size_t i = 0; i < defs.size(); ++i)
        allDeps[i] = defs[i].body->callDependencies();

    std::map<FuncKey, std::size_t, std::less<>> batchIndex;
    for (std::size_t i = 0; i < defs.size(); ++i)
        batchIndex.emplace(FuncKey{defs[i].name, defs[i].params.size()}, i);

    for (std::size_t i = 0; i < defs.size(); ++i)
    {
        auto const& def = defs[i];
        FuncKey key{def.name, def.params.size()};

        auto bi = builtins.find(def.name);
        if (bi != builtins.end() && bi->second.arity == static_cast<int>(def.params.size()))
            return std::unexpected(EvalError{std::format("cannot shadow builtin function: {}", def.name)});

        if (allDeps[i].count(key) > 0)
            return std::unexpected(EvalError{"recursion not allowed"});

        for (auto const& dep : allDeps[i])
        {
            bool found = false;
            auto builtin_it = builtins.find(dep.first);
            if (builtin_it != builtins.end() && builtin_it->second.arity == static_cast<int>(dep.second))
                found = true;
            if (!found && userFunctions_.count(dep) > 0)
                found = true;
            if (!found && batchIndex.count(dep) > 0)
                found = true;
            if (!found)
                return std::unexpected(EvalError{
                    std::format("undefined function reference: {}/{}", dep.first, dep.second)});
        }

        auto varRefs = def.body->variableReferences();
        for (auto const& param : def.params)
            varRefs.erase(param);
        for (auto const& varName : varRefs)
        {
            if (!getVariables().get(varName).has_value())
                return std::unexpected(EvalError{std::format("undefined variable in function body: {}", varName)});
        }
    }

    auto newDepGraph = dependencies_;
    for (std::size_t i = 0; i < defs.size(); ++i)
        newDepGraph[FuncKey{defs[i].name, defs[i].params.size()}] = allDeps[i];

    for (std::size_t i = 0; i < defs.size(); ++i)
    {
        if (hasCycle(FuncKey{defs[i].name, defs[i].params.size()}, newDepGraph))
            return std::unexpected(EvalError{"recursion not allowed"});
    }

    for (std::size_t i = 0; i < defs.size(); ++i)
    {
        FuncKey key{defs[i].name, defs[i].params.size()};
        dependencies_[key] = std::move(allDeps[i]);
        userFunctions_[key] = UserFunction{defs[i].params, defs[i].body->clone(), defs[i].source};
    }

    return {};
}

auto parseFunctionName(std::string_view name) -> std::expected<std::pair<std::string, std::optional<std::size_t>>, EvalError>
{
    auto slashPos = name.find('/');
    if (slashPos == std::string_view::npos)
        return std::pair<std::string, std::optional<std::size_t>>{std::string(name), std::nullopt};

    auto funcName = std::string(name.substr(0, slashPos));
    auto arityStr = name.substr(slashPos + 1);

    try
    {
        auto arity = std::stoul(std::string(arityStr));
        return std::pair<std::string, std::optional<std::size_t>>{funcName, arity};
    }
    catch (...)
    {
        return std::unexpected(EvalError{std::format("invalid function name: {} (non-numeric arity)", name)});
    }
}

FUNCTIONS::remove(std::span<std::string const> names) -> std::expected<void, EvalError>
{
    std::set<FuncKey, std::less<>> keysToRemove;
    for (auto const& name : names)
    {
        auto parseResult = parseFunctionName(name);
        if (!parseResult.has_value())
            return std::unexpected(parseResult.error());

        auto [funcName, arityOpt] = *parseResult;

        if (arityOpt.has_value())
        {
            FuncKey key{funcName, *arityOpt};
            if (userFunctions_.count(key) > 0)
                keysToRemove.insert(key);
        }
        else
        {
            for (const auto& [key, _] : userFunctions_)
            {
                if (key.first == funcName)
                    keysToRemove.insert(key);
            }
        }
    }

    for (const auto& [key, deps] : dependencies_)
    {
        if (keysToRemove.count(key) > 0)
            continue;

        for (const auto& dep : deps)
        {
            if (keysToRemove.count(dep) > 0)
            {
                return std::unexpected(EvalError{
                    std::format("cannot remove {}/{}: {}/{} depends on it",
                        dep.first, dep.second, key.first, key.second)
                });
            }
        }
    }

    for (const auto& key : keysToRemove)
    {
        userFunctions_.erase(key);
        dependencies_.erase(key);
    }

    return {};
}

FUNCTIONS::get(std::string_view name, std::size_t arity) const -> UserFunction const*
{
    auto it = userFunctions_.find({std::string(name), arity});
    if (it == userFunctions_.end())
        return nullptr;
    return &it->second;
}

FUNCTIONS::impl(UserFunctions::list) const -> std::vector<std::pair<std::string, UserFunction const*>>
{
    std::vector<std::pair<std::string, UserFunction const*>> result;
    result.reserve(userFunctions_.size());

    for (const auto& [key, ufn] : userFunctions_)
        result.push_back({key.first, &ufn});

    return result;
}

FUNCTIONS::clear() -> void
{
    userFunctions_.clear();
    dependencies_.clear();
}

} // namespace examples::calculator::node
