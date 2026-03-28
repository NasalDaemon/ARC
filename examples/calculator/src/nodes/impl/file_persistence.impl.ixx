module examples.calculator.file_persistence:impl;

import examples.calculator.file_persistence;
import std;

#define FILE_PERSISTENCE \
    template<class Context> \
    auto FilePersistence::Node<Context>

namespace examples::calculator::node {

FILE_PERSISTENCE::save(std::string_view path) -> std::expected<void, std::string>
{
    std::ofstream file{std::string(path)};
    if (!file.is_open())
        return std::unexpected(std::string("Cannot open file for writing: ") + std::string(path));

    auto const vars = this->getVariables().list();
    for (auto const& [name, value] : vars)
    {
        file << name << '=' << value << '\n';
        if (file.fail())
            return std::unexpected(std::string("Write error while saving to: ") + std::string(path));
    }

    file.close();
    if (file.fail())
        return std::unexpected(std::string("Failed to close file: ") + std::string(path));

    return {};
}

FILE_PERSISTENCE::load(std::string_view path) -> std::expected<void, std::string>
{
    std::ifstream file{std::string(path)};
    if (!file.is_open())
        return std::unexpected(std::string("Cannot open file for reading: ") + std::string(path));

    this->getVariables().clear();

    std::string line;
    while (std::getline(file, line))
    {
        if (line.empty())
            continue;

        auto const sep = line.find('=');
        if (sep == std::string::npos)
            continue; // skip malformed lines

        auto const name = line.substr(0, sep);
        auto const valueStr = line.substr(sep + 1);

        if (name.empty())
            continue; // skip lines with empty name

        try
        {
            std::size_t pos = 0;
            double const value = std::stod(valueStr, &pos);
            if (pos != valueStr.size())
                continue; // skip lines where value is not fully parsed
            this->getVariables().set(std::string(name), value);
        }
        catch (std::exception const&)
        {
            continue; // skip lines with unparseable values
        }
    }

    if (file.bad())
        return std::unexpected(std::string("Read error while loading from: ") + std::string(path));

    return {};
}

}
