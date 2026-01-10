module;
#include "arc/macros.hpp"
#if !ARC_IMPORT_STD
#include <iostream>
#endif
export module abc.bob;

import abc.traits;
import arc;
#if ARC_IMPORT_STD
import std;
#endif

export namespace abc {

struct Bob
{
    template<class Context>
    struct Node : arc::Node
        ::Impl<trait::AliceRead, trait::Bob, trait::Charlie>
        ::Uses<trait::Alice, trait::Charlie>
    {
        void onGraphConstructed() { std::cout << "Constructed Bob " << asBob().get() << "\n"; }

        int impl(trait::Alice::get) const
        {
            return getAlice().get();
        }

        int impl(trait::Bob::get) const { return bob; }
        void impl(trait::Bob::set, int value) { bob = value; }

        int impl(trait::Charlie::get) const
        {
            return getCharlie().get();
        }

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
