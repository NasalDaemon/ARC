#pragma once

#include "abc/traits.hpp"

#include "arc/depends.hpp"
#include "arc/node.hpp"
#include "arc/resolve.hpp"
#include "arc/traits.hpp"

namespace abc {

struct Charlie
{
    template<class Context>
    struct Node : arc::Node
    {
        using Depends = arc::Depends<trait::Alice>;

        struct Alice;
        struct Charlie;
        struct Charlie2;
        struct Charlie3;

        struct AliceTypes;
        struct CharlieTypes;

        using Traits = arc::Traits<Node
            , trait::AliceRead(Alice, AliceTypes)
            , trait::Charlie(Charlie, CharlieTypes)
            , trait::Charlie2(Charlie2, CharlieTypes)
            , trait::Charlie3(Charlie3, CharlieTypes)
            , trait::Visitable
        >;

        struct AliceTypes
        {
            using AliceType = arc::ResolveTypes<Node, trait::AliceRead>::AliceType;
        };
        struct CharlieTypes
        {
            using CharlieType = int;
        };

        using AliceType = AliceTypes::AliceType;
        using CharlieType = CharlieTypes::CharlieType;

        void impl(trait::Visitable::count, int& counter);

    protected:
        AliceType charlie = 99;
    };
};

template<class Context>
struct Charlie::Node<Context>::Alice : Node
{
    int impl(trait::Alice::get) const;
};

template<class Context>
struct Charlie::Node<Context>::Charlie : Node
{
    int impl(trait::Charlie::get) const;
};

template<class Context>
struct Charlie::Node<Context>::Charlie2 : Node
{
    int impl(trait::Charlie::get) const
    {
        return -asTrait(trait::charlie).get();
    }
};

template<class Context>
struct Charlie::Node<Context>::Charlie3 : Node
{
    static int impl(trait::Charlie::get)
    {
        return 15;
    }
};

}
