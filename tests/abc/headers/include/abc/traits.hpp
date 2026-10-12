#pragma once

#include "arc/manual/trait.hpp"

namespace abc::inline trait {

struct AliceRead : arc::Trait
{
    #define ARC_METHODS_AliceRead(TAG) \
        TAG(get) \

    ARC_METHODS(AliceRead)

    template<class Self, class Impl, class Types>
    friend consteval void assertImplements(arc::AdlTag<AliceRead>)
    {
        ARC_ASSERT_IMPLEMENTS(Self, Impl, Types, "Types must declare AliceType", requires { typename Types::AliceType; });
        ARC_ASSERT_IMPLEMENTS(Self, Impl, Types, "Impl must implement: get() const -> int",
            requires (Impl const c) { { c.impl(get_c) } -> std::same_as<int>; });
    }
} inline constexpr aliceRead{};

struct AliceWrite : arc::Trait
{
    #define ARC_METHODS_AliceWrite(TAG) \
        TAG(set) \

    ARC_METHODS(AliceWrite)

    template<class Self, class Impl, class Types>
    friend consteval void assertImplements(arc::AdlTag<AliceWrite>)
    {
        ARC_ASSERT_IMPLEMENTS(Self, Impl, Types, "Types must declare AliceType", requires { typename Types::AliceType; });
        ARC_ASSERT_IMPLEMENTS(Self, Impl, Types, "Impl must implement: set(int)", requires (Impl t, int i) { t.impl(set_c, i); });
    }
} inline constexpr aliceWrite{};

struct Bob : arc::Trait
{
    #define ARC_METHODS_Bob(TAG) \
        TAG(get) \
        TAG(set) \

    ARC_METHODS(Bob)

    template<class Self, class Impl, class Types>
    friend consteval void assertImplements(arc::AdlTag<Bob>)
    {
        ARC_ASSERT_IMPLEMENTS(Self, Impl, Types, "Types must declare BobType", requires { typename Types::BobType; });
        ARC_ASSERT_IMPLEMENTS(Self, Impl, Types, "Impl must implement: get() const -> int",
            requires (Impl const c) { { c.impl(get_c) } -> std::same_as<int>; });
    }
} inline constexpr bob{};

struct Charlie : arc::Trait
{
    #define ARC_METHODS_Charlie(TAG) \
        TAG(get) \

    ARC_METHODS(Charlie)

    template<class Self, class Impl, class Types>
    friend consteval void assertImplements(arc::AdlTag<Charlie>)
    {
        ARC_ASSERT_IMPLEMENTS(Self, Impl, Types, "Types must declare CharlieType", requires { typename Types::CharlieType; });
        ARC_ASSERT_IMPLEMENTS(Self, Impl, Types, "Impl must implement: get() const -> int",
            requires (Impl const c) { { c.impl(get_c) } -> std::same_as<int>; });
    }
} inline constexpr charlie{};

struct Ellie : arc::Trait
{
    #define ARC_METHODS_Ellie(TAG) \
        TAG(get) \

    ARC_METHODS(Ellie)

    template<class Self, class Impl, class Types>
    friend consteval void assertImplements(arc::AdlTag<Ellie>)
    {
        ARC_ASSERT_IMPLEMENTS(Self, Impl, Types, "Impl must implement: get() const -> int",
            requires (Impl const c) { { c.impl(get_c) } -> std::same_as<int>; });
    }
} inline constexpr ellie{};

struct Visitable : arc::Trait
{
    #define ARC_METHODS_Visitable(TAG) \
        TAG(count) \

    ARC_METHODS(Visitable)
} inline constexpr visitable{};

using Alice = arc::JoinedTrait<AliceRead, AliceWrite>;
using Charlie2 = arc::AltTrait<Charlie, struct Charlie_Tag2>;
using Charlie3 = arc::AltTrait<Charlie, struct Charlie_Tag3>;
using Ellie2 = arc::AltTrait<Ellie, struct Ellie_Tag2>;
using Ellie3 = arc::AltTrait<Ellie, struct Ellie_Tag3>;
inline constexpr Alice alice{};
inline constexpr Charlie2 charlie2{};
inline constexpr Charlie3 charlie3{};
inline constexpr Ellie2 ellie2{};
inline constexpr Ellie3 ellie3{};

}

#undef ARC_METHODS_AliceRead
#undef ARC_METHODS_AliceWrite
#undef ARC_METHODS_Bob
#undef ARC_METHODS_Charlie
