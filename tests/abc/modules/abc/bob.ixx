module;
#if !ARC_IMPORT_STD
#include <cstdio>
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
    {
        using Depends = arc::Depends<trait::Alice, trait::Charlie>;

        using Traits = arc::Traits<Node
            , trait::AliceRead
            , trait::Bob
            , trait::Charlie
        >;

        void onGraphConstructed() { std::puts("Constructed Bob"); }

        int impl(trait::Alice::get) const
        {
            return getNode(trait::alice).get();
        }

        int impl(trait::Bob::get) const { return bob; }
        void impl(trait::Bob::set, int value) { bob = value; }

        int impl(trait::Charlie::get) const
        {
            return getNode(trait::charlie).get();
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
