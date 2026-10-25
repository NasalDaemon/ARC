export module arc.tests.trait_requires.literals;

namespace arc::tests::trait_requires {

trait Literals
{
    requires requires { { impl_.f("}", '(', 1'000) } -> std::same_as<int>; }

    f(std::string_view s, char c, int n) -> int
    url() const -> std::string_view { return "http://x/*"; }
    slashes() const -> std::integral_constant<std::size_t, sizeof("//")> { return std::integral_constant<std::size_t, sizeof("//")>{}; }
    isComment(char c) const -> bool
        pre (c != "/*"[0])
    {
        return c == "/*"[1];
    }
}

} // namespace arc::tests::trait_requires
