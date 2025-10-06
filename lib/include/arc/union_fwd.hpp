#ifndef INCLUDE_ARC_UNION_FWD_HPP
#define INCLUDE_ARC_UNION_FWD_HPP

namespace arc {

namespace detail {
    struct IsUnionContextTag {};
}

ARC_MODULE_EXPORT
template<class Context>
concept IsUnionContext = IsContext<Context> and requires {
    { Context::isUnionContext(detail::IsUnionContextTag()) } -> std::same_as<detail::Decompress<Context>>;
};

ARC_MODULE_EXPORT
template<IsNodeHandle... Options>
struct Union;

} // namespace arc

#endif // INCLUDE_ARC_UNION_FWD_HPP
