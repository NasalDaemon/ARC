#ifndef INCLUDE_ARC_GLOBAL_CONTEXT_HPP
#define INCLUDE_ARC_GLOBAL_CONTEXT_HPP

#include "arc/context_fwd.hpp"
#include "arc/detail/cast.hpp"
#include "arc/detail/compress.hpp"
#include "arc/global_trait.hpp"
#include "arc/link.hpp"
#include "arc/macros.hpp"
#include "arc/trait_view.hpp"

namespace arc {

ARC_MODULE_EXPORT
template<class Context>
concept ContextHasGlobal = IsContext<Context> and requires {
    typename Context::Info::GlobalNode;
};

ARC_MODULE_EXPORT
template<class Context, class GlobalTrait>
concept ContextHasGlobalTrait = ContextHasGlobal<Context> and IsGlobalTrait<GlobalTrait> and HasTrait<typename Context::Info::GlobalNode, typename GlobalTrait::Trait>;

namespace detail {

    template<IsContext Context, IsGlobalTrait GlobalTrait>
    consteval void assertContextHasGlobalTrait()
    {
        static_assert(ContextHasGlobal<Context>, "Context does not have a global node or cluster");
        static_assert(ContextHasGlobalTrait<Context, GlobalTrait>, "Global node or cluster does not have the requested trait");
    }

    template<class Context>
    struct GlobalNodePtr
    {
        constexpr GlobalNodePtr(void*) {}

        static void* get() { return nullptr; }
        static void set(void*) {}
    };

    template<ContextHasGlobal Context>
    struct GlobalNodePtr<Context>
    {
        using Node = ContextToNode<Decompress<Context>>;
        using GlobalNode = Context::Info::GlobalNode;
        constexpr GlobalNodePtr(Node* node)
        {
            set(node);
        }

        ARC_INLINE constexpr auto* get() const { return ptr; }
        ARC_INLINE constexpr void set(Node* node) { ptr = std::addressof(upCast<GlobalNode>(Context{}.getGlobalNode(*node))); }
    private:
        GlobalNode* ptr = nullptr;
    };
}

} // namespace arc


#endif // INCLUDE_ARC_GLOBAL_CONTEXT_HPP
