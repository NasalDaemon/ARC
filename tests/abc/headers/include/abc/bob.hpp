#pragma once

#include "abc/traits.hpp"

#include "arc/depends.hpp"
#include "arc/node.hpp"
#include "arc/resolve.hpp"
#include "arc/traits.hpp"

namespace abc {

struct Bob
{
    template<class Context>
    struct Node : arc::Node
    {
        using Depends = arc::Depends<trait::Alice, trait::Charlie>;

        using Traits = arc::Traits<Node
            , trait::AliceRead
            , trait::Bob
            , trait::Charlie
        >;

        int impl(trait::Alice::get) const;

        int impl(trait::Bob::get) const;
        void impl(trait::Bob::set, int value);

        int impl(trait::Charlie::get) const;

        struct Types
        {
            using BobType = int;
            using CharlieType = arc::ResolveTypes<Node, trait::Charlie>::CharlieType;
        };

        using AliceType = arc::ResolveTypes<Node, trait::Alice>::AliceType;
        using BobType = Types::BobType;
        using CharlieType = Types::CharlieType;

    private:
        AliceType bob = 64;
    };
};

}
