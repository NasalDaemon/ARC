#ifndef INCLUDE_ARC_DETAIL_WITH_INDEX_HPP
#define INCLUDE_ARC_DETAIL_WITH_INDEX_HPP

#include "arc/macros.hpp"

#if !ARC_IMPORT_STD
#include <algorithm>
#include <cstddef>
#include <functional>
#include <type_traits>
#include <utility>
#endif

namespace arc::detail {

template<std::size_t Count, std::size_t Index>
struct WithIndexInvokeTag
{
    static constexpr std::size_t count = Count;
    static constexpr std::size_t index = Index;

    static consteval auto getIndexTag()
    {
        static_assert(count == 1);
        return std::integral_constant<std::size_t, Index>{};
    };

    template<std::size_t I, std::size_t Multiplier = 1, bool Tail = false>
    friend consteval auto nextTag(WithIndexInvokeTag)
    {
        constexpr auto IM = I * Multiplier;
        constexpr auto clampedCount = Tail
            ? std::max(Count, IM)
            : std::clamp(Count, IM, IM + Multiplier);
        constexpr auto nextCount = clampedCount - IM;
        constexpr auto nextIndex = nextCount == 0 ? 0 : Index + IM;
        return WithIndexInvokeTag<nextCount, nextIndex>{};
    }
};

template<class Result, std::size_t Index, class Visitor>
constexpr Result withIndexInvoke(WithIndexInvokeTag<0, Index>, std::size_t, Visitor&&)
{
    std::unreachable();
}

template<class Result, std::size_t Index, class Visitor>
constexpr Result withIndexInvoke(WithIndexInvokeTag<1, Index> tag, std::size_t, Visitor&& visitor)
{
    return std::invoke(ARC_FWD(visitor), tag.getIndexTag());
}

template<class Result, class Visitor>
constexpr Result withIndexInvoke(auto tag, std::size_t i, Visitor&& visitor)
{
    [[assume(i < tag.count)]];
    if constexpr (tag.count <= 8)
    {
        switch (i)
        {
        case 0: return withIndexInvoke<Result>(nextTag<0>(tag), 0, ARC_FWD(visitor));
        case 1: return withIndexInvoke<Result>(nextTag<1>(tag), 0, ARC_FWD(visitor));
        case 2: return withIndexInvoke<Result>(nextTag<2>(tag), 0, ARC_FWD(visitor));
        case 3: return withIndexInvoke<Result>(nextTag<3>(tag), 0, ARC_FWD(visitor));
        case 4: return withIndexInvoke<Result>(nextTag<4>(tag), 0, ARC_FWD(visitor));
        case 5: return withIndexInvoke<Result>(nextTag<5>(tag), 0, ARC_FWD(visitor));
        case 6: return withIndexInvoke<Result>(nextTag<6>(tag), 0, ARC_FWD(visitor));
        case 7: return withIndexInvoke<Result>(nextTag<7>(tag), 0, ARC_FWD(visitor));
        }
        std::unreachable();
    }
    else
    {
        // Bucket: bit_ceil(count/3) with an upper bound of 8
        constexpr std::size_t m = std::min(static_cast<decltype(tag.count)>(8), std::bit_ceil(1 + ((tag.count - 1) / 3)));
        switch (i / m)
        {
        case 0:  return withIndexInvoke<Result>(nextTag<0, m, false>(tag), i,       ARC_FWD(visitor));
        case 1:  return withIndexInvoke<Result>(nextTag<1, m, false>(tag), i - m,   ARC_FWD(visitor));
        default: return withIndexInvoke<Result>(nextTag<2, m, true >(tag), i - m*2, ARC_FWD(visitor));
        }
    }
}

template<std::size_t Count>
requires (Count > 0)
struct WithIndex
{
    template<class Visitor, std::size_t... Is>
    static auto commonType(std::index_sequence<Is...>) -> std::common_reference_t<std::invoke_result_t<Visitor, std::integral_constant<std::size_t, Is>>...>;

    template<class Visitor>
    using Result = decltype(commonType<Visitor>(std::make_index_sequence<Count>{}));

    template<class Visitor>
    constexpr Result<Visitor> operator()(std::size_t i, Visitor&& visitor) const
    {
        return withIndexInvoke<Result<Visitor>>(
            WithIndexInvokeTag<Count, 0>(),
            i,
            ARC_FWD(visitor));
    }
};

}

namespace arc {
    ARC_MODULE_EXPORT
    template<std::size_t Count>
    inline constexpr detail::WithIndex<Count> withIndex{};
}


#endif // INCLUDE_ARC_DETAIL_WITH_INDEX_HPP
