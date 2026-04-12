module examples.calculator.node.file_persistence:impl;

import examples.calculator.node.file_persistence;
import examples.calculator.types;
import examples.calculator.traits;
import std;

#define FILE_PERSISTENCE \
    template<class Context> \
    auto FilePersistence::Node<Context>

namespace examples::calculator::node {

FILE_PERSISTENCE::save(std::string_view path) -> std::expected<void, std::string>
{
    std::ofstream file{std::string(path)};
    if (!file.is_open())
        return std::unexpected(std::format("Cannot open file for writing: {}", path));

    auto const vars = getVariables().list();
    for (auto const& [name, value] : vars)
    {
        file << name << '=' << value << '\n';
        if (file.fail())
            return std::unexpected(std::format("Write error while saving to: {}", path));
    }

    // Write user-defined functions
    auto userFuncs = getFunctions().listUserFunctions();
    for (auto const& [name, func] : userFuncs)
    {
        if (func)
        {
            // Format: fn:name(param1,param2,...)=body_source
            file << "fn:" << name << "(";
            for (std::size_t i = 0; i < func->params.size(); ++i)
            {
                if (i > 0) file << ",";
                file << func->params[i];
            }
            file << ")=" << func->source << '\n';
        }
        if (file.fail())
            return std::unexpected(std::format("Write error while saving to: {}", path));
    }

    file.close();
    if (file.fail())
        return std::unexpected(std::format("Failed to close file: {}", path));

    return {};
}

FILE_PERSISTENCE::load(std::string_view path) -> std::expected<void, std::string>
{
    std::ifstream file{std::string(path)};
    if (!file.is_open())
        return std::unexpected(std::format("Cannot open file for reading: {}", path));

    getVariables().clear();
    getFunctions().clearUserFunctions();

    std::string line;
    std::vector<FunctionDefinition> functionDefs;

    while (std::getline(file, line))
    {
        if (line.empty())
            continue;

        if (line.size() >= 3 && line.substr(0, 3) == "fn:")
        {
            auto eqPos = line.find('=');
            if (eqPos == std::string::npos)
                return std::unexpected(std::format("Malformed function line: {}", line));

            auto fnDef  = line.substr(3, eqPos - 3);
            auto source = line.substr(eqPos + 1);

            auto parenStart = fnDef.find('(');
            auto parenEnd   = fnDef.rfind(')');
            if (parenStart == std::string::npos || parenEnd == std::string::npos)
                return std::unexpected(std::format("Malformed function definition: {}", fnDef));

            auto fname     = fnDef.substr(0, parenStart);
            auto paramsStr = fnDef.substr(parenStart + 1, parenEnd - parenStart - 1);

            std::vector<std::string> params;
            if (!paramsStr.empty())
            {
                std::istringstream iss(paramsStr);
                std::string param;
                while (std::getline(iss, param, ','))
                {
                    auto start = param.find_first_not_of(" \t");
                    auto end   = param.find_last_not_of(" \t");
                    if (start != std::string::npos)
                    {
                        param = param.substr(start, end - start + 1);
                        if (!param.empty())
                            params.push_back(param);
                    }
                }
            }

            auto tokenResult = getTokeniser().tokenise(source);
            if (!tokenResult)
                return std::unexpected(std::format("Parse error in function body: {}", source));

            auto parseResult = getParser().parse(*tokenResult, source);
            if (!parseResult)
                return std::unexpected(std::format("Parse error in function body: {}", source));

            functionDefs.push_back(
                {std::move(fname), std::move(params), std::move(*parseResult), std::move(source)});
        }
        else
        {
            auto const sep = line.find('=');
            if (sep == std::string::npos)
                continue;

            auto const name     = line.substr(0, sep);
            auto const valueStr = line.substr(sep + 1);
            if (name.empty())
                continue;

            double value = 0.0;
            auto [ptr, ec] = std::from_chars(valueStr.data(), valueStr.data() + valueStr.size(), value);
            if (ec != std::errc{} || ptr != valueStr.data() + valueStr.size())
                continue;
            getVariables().set(std::string(name), value);
        }
    }

    if (file.bad())
        return std::unexpected(std::format("Read error while loading from: {}", path));

    if (!functionDefs.empty())
    {
        auto result = getFunctions().define(std::span{functionDefs});
        if (!result)
            return std::unexpected(result.error().message);
    }

    return {};
}

} // namespace examples::calculator::node
