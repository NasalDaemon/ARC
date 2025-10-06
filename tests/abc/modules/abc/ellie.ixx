module;
#if !ARC_IMPORT_STD
#include <type_traits>
#endif
export module abc.ellie;

import abc.traits;
import arc;
#if ARC_IMPORT_STD
import std;
#endif

export namespace abc {

struct EllieType{};
struct EllieType3{};

struct Ellie : arc::Node
{
    using Depends = arc::Depends<trait::Charlie>;

    struct Charlie;
    struct Charlie2;
    struct Ellie3Types;

    using Traits = arc::Traits<Ellie
        , trait::Ellie
        , trait::Ellie3*(Ellie3Types)
        , trait::Charlie(Charlie)
        , trait::Charlie2(Charlie2)
    >;

    // Degenerate case, but just to prove that it works...
    struct Charlie2 : arc::DetachedInterface
    {
        template<class Self>
        int impl(this Self const& self, trait::Charlie::get)
        {
            using CharlieType = arc::ResolveTypes<Self, trait::Charlie>::CharlieType;
            static_assert(std::is_same_v<int, CharlieType>);
            return self.getNode(trait::charlie).get();
        }
    };
    struct Ellie3Types
    {
        using EllieType = abc::EllieType3;
    };

    struct Types
    {
        using CharlieType = int;
        using EllieType = abc::EllieType;
    };

    inline Ellie(int value = 101) : value(value) {}

    void onGraphConstructed();

    int impl(trait::Ellie::get) const;

    int value;
};

struct Ellie::Charlie : Ellie
{
    template<class Self>
    int impl(this Self const& self, trait::Charlie::get)
    {
        using CharlieType = arc::ResolveTypes<Self, trait::Charlie>::CharlieType;
        static_assert(std::is_same_v<int, CharlieType>);
        return self.getNode(trait::charlie).get();
    }
};

}
