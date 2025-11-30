module examples.filesystem.path_ops;

import std;

namespace examples::filesystem {

auto PathOps::normalise(std::string_view path) const -> std::string
{
    if (path.empty())
        return "/";

    std::string result;
    result.reserve(path.size());

    std::size_t start = 0;
    bool in_component = false;

    auto const processComponent = [&](std::string_view component) {
        if (component == "..")
        {
            // Pop last component if exists
            if (result.size() > 0)
            {
                auto pos = result.rfind('/');
                if (pos != std::string::npos)
                    result.resize(pos);
            }
        }
        else if (component != ".")
        {
            result += '/';
            result += component;
        }
    };

    for (std::size_t i = 0; i < path.size(); ++i)
    {
        if (path[i] == '/')
        {
            if (in_component)
            {
                processComponent(path.substr(start, i - start));
                in_component = false;
            }
        }
        else if (not in_component)
        {
            start = i;
            in_component = true;
        }
    }

    // Handle trailing component
    if (in_component)
        processComponent(path.substr(start));

    if (result.empty())
        result += '/';

    return result;
}

auto PathOps::impl(trait::PathOps::parent, std::string_view path) const -> std::string
{
    std::string normalised = normalise(path);
    auto const pos = normalised.rfind('/');
    if (pos == 0)
        normalised.resize(1);
    else if (pos == std::string::npos)
        normalised = "/";
    else
        normalised.resize(pos);
    return normalised;
}

auto PathOps::impl(trait::PathOps::filename, std::string_view path) const -> std::string
{
    std::string normalised = normalise(path);
    if (normalised == "/")
        normalised.clear();
    else if (auto pos = normalised.rfind('/'); pos != std::string::npos)
        normalised.erase(0, pos + 1);

    return normalised;
}

auto PathOps::impl(trait::PathOps::join, std::string_view base, std::string_view child) const -> std::string
{
    if (child.empty())
        return normalise(base);

    if (child[0] == '/')
        return normalise(child);

    std::string result{base};
    if (not result.empty() and result.back() != '/')
        result += '/';
    result += child;

    return normalise(result);
}

auto PathOps::impl(trait::PathOps::isRoot, std::string_view path) const -> bool
{
    return normalise(path) == "/";
}

}
