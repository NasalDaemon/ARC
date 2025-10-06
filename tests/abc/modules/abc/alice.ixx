module;
#if !ARC_IMPORT_STD
#include <type_traits>
#endif
export module abc.alice;

import abc.traits;
import arc;
#if ARC_IMPORT_STD
import std;
#endif

export namespace abc {

struct Alice
{
    struct NodeBase : arc::Node
    {
        void onGraphConstructed();

        int impl(trait::Alice::get) const;
        void impl(trait::Alice::set, int value);

        void impl(trait::Visitable::visit, int& counter);

        int alice = 92;
    };

    template<class Context>
    struct Node : NodeBase
    {
        using Depends = arc::Depends<trait::Bob, trait::Charlie>;

        struct Types;

        using Traits = arc::Traits<Node
            , trait::Alice*(Types)
            , trait::Bob
            , trait::Charlie
            , trait::Visitable
        >;

        struct Types
        {
            using AliceType = int;
            using BobType = arc::ResolveTypes<Node, trait::Bob>::BobType;
            using CharlieType = arc::ResolveTypes<Node, trait::Bob>::CharlieType;
        };

        using AliceType = Types::AliceType;
        using BobType = Types::BobType;

        using NodeBase::impl;

        int impl(trait::Bob::get) const;
        void impl(trait::Bob::set, int);

        int impl(trait::Charlie::get method) const;

        static_assert(std::is_same_v<decltype(NodeBase::alice), BobType>);
        static_assert(std::is_same_v<arc::NullContext::Root, arc::ResolveRoot<Context>>);
        static_assert(std::is_same_v<arc::NullContext::Info, arc::ResolveInfo<Context>>);
    };
};

}
