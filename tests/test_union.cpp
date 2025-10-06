#include "arc/macros.hpp"

#include <doctest/doctest.h>

#if !ARC_IMPORT_STD
#include <concepts>
#include <cstdio>
#include <utility>
#endif

import arc;

namespace arc::tests::union_ {

namespace trait {

    struct Name : arc::Trait
    {
        #define ARC_METHODS_Name(TAG) \
            TAG(get) \

        ARC_METHODS(Name)

        template<class Self, class T, class Types>
        requires requires (T const c)
        {
            { c.impl(get_c) } -> std::same_as<int>;
        }
        using Implements = void;
    };

} // trait

struct MouseType;

struct Cat : arc::Node
{
    using Traits = arc::Traits<Cat, trait::Name>;
    struct Types
    {
        struct CatType;
    };

    int impl(this auto& self, trait::Name::get)
    {
        return self.getNode(trait::Name{}).get() + 99;
    }
};
struct Dog
{
    template<class Context>
    struct Node : arc::Node
    {
        using Traits = arc::Traits<Node, trait::Name>;
        struct Types
        {
            struct DogType;
        };

        static_assert(std::is_same_v<MouseType, typename arc::ResolveTypes<Node, trait::Name>::MouseType>);

        int impl(trait::Name::get) const
        {
            return getNode(trait::Name{}).get();
        }
    };
};
template<class Context>
struct Mouse : arc::Node
{
    using Traits = arc::Traits<Mouse, trait::Name>;
    struct Types
    {
        using MouseType = union_::MouseType;
    };

    int impl(trait::Name::get) const { return 42; }
};

template<class Context>
struct Union : arc::Cluster
{
    struct Onion;
    struct Mouse;

    ARC_LINK(trait::Name, Onion)

    struct Onion : arc::Context<Union, arc::Union<Cat, Dog>>
    {
        ARC_LINK(trait::Name, Mouse)
    };
    struct Mouse : arc::InlineContext<Union, union_::Mouse>
    {
        ARC_LINK(trait::Name, Onion)
    };

    ARC_NODE(Onion, onion)
    ARC_NODE(Mouse, mouse)
};

constexpr bool testWithIndex()
{
    for (int i = 0; i < 24; ++i)
    {
        if (i != arc::withIndex<24>(i, [](auto index) -> int { return index; }))
            return false;
    }
    return true;
}

static_assert(testWithIndex());

TEST_CASE("arc::Union")
{
    arc::InlineGraph<Union> cat{.onion{std::in_place_index<0>}};
    arc::InlineGraph<Union> dog{.onion{arc::withFactory, [](auto c) { return c(std::in_place_type<Dog>); }}};

    using DogTypes = decltype(dog.onion.asTrait(trait::Name{}))::Types;
    static_assert(DogTypes::TypesCount == 2);
    static_assert(requires { typename DogTypes::TypesAt<0>::CatType; });
    static_assert(requires { typename DogTypes::TypesAt<1>::DogType; });

    CHECK(42 == cat.mouse.asTrait(trait::Name{}).get());

    CHECK(141 == cat.onion.asTrait(trait::Name{}).get());

    CHECK(141 == cat.mouse.getNode(trait::Name{}).get());

    CHECK(42 == dog.mouse.asTrait(trait::Name{}).get());

    CHECK(42 == dog.mouse.getNode(trait::Name{}).get());

    CHECK(42 == dog.onion.asTrait(trait::Name{}).get());

    CHECK(cat.mouse.asTrait(trait::Name{})->visit([]<class T>(T) { return requires { typename T::Types::MouseType; }; }));

    CHECK(dog.mouse.asTrait(trait::Name{})->visit([]<class T>(T) { return requires { typename T::Types::MouseType; }; }));

    CHECK(cat.onion.asTrait(trait::Name{})->visit([]<class T>(T) { return requires { typename T::Types::CatType; }; }));

    CHECK(dog.onion.asTrait(trait::Name{})->visit([]<class T>(T) { return requires { typename T::Types::DogType; }; }));

    {
        arc::Defer keepAlive;
        {
            auto d = cat.onion->getIf<0>()->exchangeImpl<Dog>();
            CHECK(cat.onion.asTrait(trait::Name{}).get() == 141);
            keepAlive = std::move(d);
        }
        CHECK(cat.onion.asTrait(trait::Name{}).get() == 141);
    }
    CHECK(cat.onion.asTrait(trait::Name{}).get() == 42);
}

} // arc::tests::union_

ARC_INSTANTIATE(arc::InlineGraph<arc::tests::union_::Union>, onion->get<1>())
