#ifndef INCLUDE_ARC_THREAD_HPP
#define INCLUDE_ARC_THREAD_HPP

#include "arc/detail/cast.hpp"

#include "arc/context_fwd.hpp"
#include "arc/cluster.hpp"
#include "arc/detached.hpp"
#include "arc/detail/compress.hpp"
#include "arc/environment.hpp"
#include "arc/global_context.hpp"
#include "arc/key.hpp"
#include "arc/link.hpp"
#include "arc/macros.hpp"
#include "arc/map_info.hpp"
#include "arc/node_fwd.hpp"
#include "arc/trait.hpp"
#include "arc/traits/scheduler.hpp"

#if !ARC_IMPORT_STD
#include <cstddef>
#if ARC_COMPILER_LT(GCC, 15)
#include <string>
#else
#include <format>
#endif
#include <memory>
#include <stdexcept>
#include <tuple>
#include <utility>
#endif

namespace arc {

ARC_MODULE_EXPORT
struct ThreadEnvironment
{
    static constexpr std::size_t AnyThreadId = std::size_t(-1);
    static constexpr std::size_t DynamicThreadId = std::size_t(-2);
    static constexpr std::size_t MainThreadId = std::size_t(0);

    struct Tag{};

    template<std::size_t Id>
    struct WithId
    {
        using Tag = ThreadEnvironment::Tag;
        static WithId resolveEnvironment(Tag);

        static constexpr bool isDynamic() { return Id == ThreadEnvironment::DynamicThreadId; }

        static constexpr std::size_t ThreadId = Id;
    };
};

ARC_MODULE_EXPORT
template<std::size_t ThreadId>
constexpr auto& withThread(auto& t)
{
    return mergeEnvParts<ThreadEnvironment::WithId<ThreadId>>(t);
}

namespace detail {

    template<class Source>
    consteval std::size_t getEnvStaticThread()
    {
        if constexpr (Source::Environment::template HasTag<ThreadEnvironment::Tag>)
            return Source::Environment::template Get<ThreadEnvironment::Tag>::ThreadId;
        return ThreadEnvironment::AnyThreadId;
    }

    template<class Source>
    consteval std::size_t getCurrentStaticThread()
    {
        if constexpr (constexpr auto threadId = getEnvStaticThread<Source>(); threadId != ThreadEnvironment::AnyThreadId)
            return threadId;

        // If source is a trait (i.e. not the node's or cluster's state), then we can assume
        // access has been already granted, and the required thread is the current thread
        if constexpr (requires { ContextOf<Source>::Info::RequiredThreadId; })
            if constexpr (not IsCluster<Source> and not IsNodeState<Source>)
                return ContextOf<Source>::Info::RequiredThreadId;

        // Don't assume any current thread (not even main thread)
        return ThreadEnvironment::AnyThreadId;
    }

    template<class Target>
    consteval std::size_t getTargetStaticRequiredThread()
    {
        if constexpr (requires { ContextOf<Target>::Info::RequiredThreadId; })
            return ContextOf<Target>::Info::RequiredThreadId;

        // Require main thread for targets that do not specify a required thread
        return ThreadEnvironment::MainThreadId;
    }

    template<class Target>
    ARC_INLINE constexpr std::size_t getTargetDynamicRequiredThread(Target& target)
    {
        constexpr auto staticThreadId = getTargetStaticRequiredThread<Target>();
        if constexpr (staticThreadId != ThreadEnvironment::DynamicThreadId)
            return staticThreadId;

        using DynThreadContext = ContextOf<Target>::Info::DynThreadContext;
        using TargetNode = detail::UnderlyingNode<Target>;
        return ContextOf<Target>{}.template getParentNode<DynThreadContext>(upCast<TargetNode>(target)).getThreadId();
    }


    template<class Source, class Target>
    consteval bool sameThreadContext()
    {
        if constexpr (requires { typename ContextOf<Source>::Info::DynThreadContext; })
        {
            using STC = ContextOf<Source>::Info::DynThreadContext;
            using TTC = ContextOf<Target>::Info::DynThreadContext;
            // Don't assert if Source and Target have the same dynamic thread context type.
            // Entering another instance with an identical dynamic thread context type via a hairpin dependency
            // e.g. by exiting a collection and reentering a different element with another thread affinity --
            // is disallowed via static_assert. As hairpin dependencies are not allowed, it is safe to elide
            // the assertion as there is no way to access another instance's context in a single getNode call.
            return std::is_same_v<STC, TTC>;
        }
        return false;
    }

    template<std::size_t ThreadId>
    struct RequireThread
    {
        static_assert(ThreadId != ThreadEnvironment::DynamicThreadId, "arc::OnThread cannot be used with dynamic thread id, use arc::OnDynThread instead");

        static consteval bool anyThreadId() { return ThreadId == ThreadEnvironment::AnyThreadId; }

        template<IsContext Context>
        struct MapInfo : Context::Info
        {
            static constexpr std::size_t RequiredThreadId = ThreadId;

            using Context::Info::implicitDependencyAllowed;
            // Allow global scheduler trait to be omitted from Depends list of subnodes
            static void implicitDependencyAllowed(arc::Global<trait::Scheduler>);

            // Protection to be applied when entering or exiting this thread context
            // Does not assert itself, but ensures that thread keys are used to access the target,
            // which are responsible for asserting the correct thread
            template<class Source, class Target>
            static consteval auto requiresKeysToTarget()
            {
                auto parentRequires = Context::Info::template requiresKeysToTarget<Source, Target>();

                // Target’s static requirement (Main thread if unspecified)
                constexpr auto requiredThreadId = detail::getTargetStaticRequiredThread<Target>();
                if constexpr (requiredThreadId == ThreadEnvironment::AnyThreadId)
                {
                    return parentRequires;
                }
                // If Source already carries the correct thread affinity, don’t force a key
                else if constexpr (detail::getEnvStaticThread<Source>() == requiredThreadId)
                {
                    return parentRequires;
                }
                // If Target already carries the correct thread affinity, a thread key has already been used
                else if constexpr (detail::getEnvStaticThread<Target>() == requiredThreadId)
                {
                    return parentRequires;
                }
                // Otherwise require a thread key
                else
                {
                    return std::tuple_cat(std::tuple(ThreadEnvironment::Tag{}), parentRequires);
                }
            }

            template<class Target>
            static constexpr void assertAccessible(Target& target)
            {
                if constexpr (not anyThreadId())
                {
                    using Environment = Target::Environment;
                    static_assert(Environment::template HasTag<ThreadEnvironment::Tag>, "Access denied from unknown thread");
                    constexpr auto threadId = Environment::template Get<ThreadEnvironment::Tag>::ThreadId;
                    static_assert(threadId == RequiredThreadId, "Access denied from current thread");
                }
                Context::Info::assertAccessible(target);
            }
            template<class Target>
            static constexpr void assertVisitable(Target& target)
            {
                // If visiting the OnThread node, it handles visiting from any thread
                if constexpr (not anyThreadId() and not IsSameContext<ContextOf<Target>, Context>)
                    Context::Info::assertAccessible(target);

                Context::Info::assertVisitable(target);
            }
        };
    };

} // namespace detail

ARC_MODULE_EXPORT
template<IsNodeHandle NodeHandle, std::size_t ThreadId>
class OnThread
{
    template<class Context>
    using InnerNode = MapInfo<NodeHandle, detail::RequireThread<ThreadId>>::template Node<Context>;

public:
    template<class Context>
    struct Node : InnerNode<Context>
    {
        using InnerNode<Context>::InnerNode;

        constexpr void visit(this auto& self, auto&& visitor)
        {
            if constexpr (ContextHasGlobalTrait<Context, Global<trait::Scheduler>>)
            {
                auto scheduler = self.getGlobal(trait::scheduler);
                if (ThreadId == ThreadEnvironment::AnyThreadId or scheduler.isCurrentThread(ThreadId))
                    self.InnerNode<Context>::visit(ARC_FWD(visitor));
                else
                    scheduler.postTask(
                        ThreadId,
                        [&self, visitor = ARC_FWD(visitor)]() mutable
                        {
                            arc::withThread<ThreadId>(detail::upCast<OnThread::InnerNode<Context>>(self)).visit(std::move(visitor));
                        });
            }
            else
            {
                self.InnerNode<Context>::visit(ARC_FWD(visitor));
            }
        }
    };
};

ARC_MODULE_EXPORT
template<IsNodeHandle Node>
using AnyThread = OnThread<Node, ThreadEnvironment::AnyThreadId>;

namespace key {
    ARC_MODULE_EXPORT
    struct DynThreadAssert : Default
    {
        template<class... Tags>
        static constexpr void assertCanAcquireAccess()
        {
            if constexpr (sizeof...(Tags) > 0)
                static_assert((... or std::is_same_v<Tags, ThreadEnvironment::Tag>),
                    "arc::key::DynThreadAssert can only acquire thread affinity; "
                    "another key is required to acquire access to the target. Put access keys first.");
        }

        template<class Source, class Target>
        static constexpr auto& acquireAccess(Source&, Target& target)
        {
            using Environment = Source::Environment;

            constexpr bool threadAffinityObtained = threadAffinityAlreadyObtained<Source, Target>();
            // Don't assert eagerly on detached nodes--they will assert when they access the node's state
            if constexpr (IsDetachedImpl<Target>)
            {
                // If the source has not already obtained the thread affinity, we need to declare this in the environment
                // So that downstream nodes know that the correct thread affinity still needs to be asserted
                constexpr auto threadId = threadAffinityObtained ? ThreadEnvironment::DynamicThreadId : ThreadEnvironment::AnyThreadId;
                using Env = Environment::template InsertOrReplace<ThreadEnvironment::WithId<threadId>>;
                return detail::downCast<arc::TransferEnv<Env, Target>>(target);
            }
            else
            {
                // Assert if there is no proof that the thread affinity has been obtained
                if constexpr (not threadAffinityObtained)
                    ContextOf<Target>::Info::assertAccessible(target);

                using Env = Environment::template InsertOrReplace<ThreadEnvironment::WithId<ThreadEnvironment::DynamicThreadId>>;
                return detail::downCast<arc::TransferEnv<Env, Target>>(target);
            }
        }

    private:
        template<class Source, class Target>
        static constexpr bool threadAffinityAlreadyObtained()
        {
            if constexpr (not detail::sameThreadContext<Source, Target>())
            {
                // Source from different thread context; must assert accessibility
                return false;
            }
            else if constexpr (IsCluster<Source> or detail::IsNodeState<Source>)
            {
                // Cannot rely on cluster or node state to have obtained the correct thread affinity
                return false;
            }
            else if constexpr (IsDetachedImpl<Source> and detail::getEnvStaticThread<Target>() != ThreadEnvironment::DynamicThreadId)
            {
                // Detached nodes do not eagerly assert accessibility, only when they access the node's state
                // Therefore we rely on the environment to tell us if the thread affinity has been obtained
                // Environment::ThreadId is only DynamicThreadId if the thread affinity has provably been obtained from some prior source
                return false;
            }
            return true;
        }
    } inline constexpr dynThreadAssert{};
}

ARC_MODULE_EXPORT
template<IsNodeHandle NodeHandle>
struct OnDynThread
{
    template<class Context>
    class Node : public Cluster
    {
        struct Inner : arc::Context<Node, NodeHandle>
        {
            static constexpr std::size_t Depth = Context::Depth;

            template<class T>
            requires detail::HasLink<Context, T>
            static auto resolveLink(T) -> ResolvedLink<Context, T>;

            struct Info : Context::Info
            {
                using Context::Info::implicitDependencyAllowed;
                // Allow global scheduler trait to be omitted from Depends list of subnodes
                static void implicitDependencyAllowed(arc::Global<trait::Scheduler>);

                using DefaultKey = key::DynThreadAssert;

                using DynThreadContext = Context;
                static constexpr std::size_t RequiredThreadId = ThreadEnvironment::DynamicThreadId;

                // Protection to be applied when entering or exiting this thread context
                // Does not assert itself, but ensures that thread keys are used to access the target,
                // which are responsible for asserting the correct thread
                template<class Source, class Target>
                static consteval auto requiresKeysToTarget()
                {
                    auto parentRequires = Context::Info::template requiresKeysToTarget<Source, Target>();

                    // Target’s static requirement (Main thread if unspecified)
                    constexpr auto requiredThreadId = detail::getTargetStaticRequiredThread<Target>();
                    if constexpr (requiredThreadId == ThreadEnvironment::AnyThreadId)
                    {
                        return parentRequires;
                    }
                    // If Target already carries some kind of thread affinity (even AnyId), a thread key has already been applied
                    else if constexpr (Target::Environment::template HasTag<ThreadEnvironment::Tag>)
                    {
                        return parentRequires;
                    }
                    // Otherwise require a thread key
                    else
                    {
                        return std::tuple_cat(std::tuple(ThreadEnvironment::Tag{}), parentRequires);
                    }
                }

                template<class Target>
                static constexpr void assertAccessible(Target& target)
                {
                    auto const requiredThreadId = detail::getTargetDynamicRequiredThread(target);
                    if (requiredThreadId != ThreadEnvironment::AnyThreadId)
                    {
                        if (not target.getGlobal(trait::scheduler).isCurrentThread(requiredThreadId)) [[unlikely]]
                        #if ARC_COMPILER_LT(GCC, 15)
                            throw std::runtime_error("Access denied to node with thread affinity " + std::to_string(requiredThreadId) + " from current thread: " + target.getGlobal(trait::scheduler).currentThreadDetails());
                        #else
                            throw std::runtime_error(std::format("Access denied to node with thread affinity {} from current thread: {}", requiredThreadId, target.getGlobal(trait::scheduler).currentThreadDetails()));
                        #endif
                    }

                    Context::Info::assertAccessible(target);
                }
                template<class Target>
                static constexpr void assertVisitable(Target& target)
                {
                    assertAccessible(target);
                    Context::Info::assertVisitable(target);
                }
            };
        };

        ARC_NODE(Inner, node);
        std::size_t threadId;

    public:
        static constexpr bool isUnary() { return decltype(node)::isUnary(); }

        std::size_t getThreadId() const { return threadId; }

        constexpr explicit Node(std::size_t threadId, auto&&... args)
            : node(ARC_FWD(args)...), threadId(threadId)
        {
            if (threadId == ThreadEnvironment::DynamicThreadId)
                throw std::invalid_argument("arc::OnDynThread cannot be constructed with dynamic thread id");
        }

        template<class T>
        requires HasTrait<decltype(node), T>
        static auto resolveLink(T) -> ResolvedLink<Inner, T>;

        constexpr auto* operator->(this auto& self)
        {
            return std::addressof(self.node);
        }

        constexpr auto& get(this auto& self)
        {
            return self.node;
        }

        constexpr void visit(this auto& self, auto&& visitor)
        {
            auto scheduler = self.getGlobal(trait::scheduler);
            if (self.threadId == ThreadEnvironment::AnyThreadId or scheduler.isCurrentThread(self.threadId))
                self.node.visit(ARC_FWD(visitor));
            else
                scheduler.postTask(
                    self.threadId,
                    [&self, visitor = ARC_FWD(visitor)]() mutable
                    {
                        arc::withThread<ThreadEnvironment::DynamicThreadId>(self.node).visit(std::move(visitor));
                    });
        }
    };
};

namespace key {

    ARC_MODULE_EXPORT
    template<class Poster>
    struct ThreadPost
    {
        template<class T>
        using Trait = arc::detail::DuckTrait<T>;

        template<class T, std::size_t CurrentThreadId>
        struct Interface : T
        {
            static_assert(std::derived_from<Poster, ThreadPost>);
            constexpr decltype(auto) visit(this auto& self, auto&& visitor)
            {
                std::size_t dynamicRequiredThreadId = requiredThreadId;
                if constexpr (requiredThreadId == ThreadEnvironment::DynamicThreadId)
                    dynamicRequiredThreadId = detail::getTargetDynamicRequiredThread(self);
                return Poster::template post<CurrentThreadId, requiredThreadId>(
                    self.getGlobal(trait::scheduler),
                    dynamicRequiredThreadId,
                    [&, visitor = ARC_FWD(visitor)](this auto&&) -> decltype(auto)
                    {
                        return self.T::visit(std::move(visitor));
                    });
            }

            constexpr decltype(auto) impl(this auto& self, auto&&... args)
            {
                std::size_t dynamicRequiredThreadId = requiredThreadId;
                if constexpr (requiredThreadId == ThreadEnvironment::DynamicThreadId)
                    dynamicRequiredThreadId = detail::getTargetDynamicRequiredThread(self);
                return Poster::template post<CurrentThreadId, requiredThreadId>(
                    self.getGlobal(trait::scheduler),
                    dynamicRequiredThreadId,
                    [&self, ...args = ARC_FWD(args)](this auto&&) -> decltype(auto)
                    {
                        return self.T::impl(std::move(args)...);
                    });
            }

        private:
            static constexpr auto requiredThreadId = ContextOf<T>::Info::RequiredThreadId;
        };

        template<class... Tags>
        static constexpr void assertCanAcquireAccess()
        {
            if constexpr (sizeof...(Tags) > 0)
                static_assert((... or std::is_same_v<Tags, ThreadEnvironment::Tag>),
                    "arc::key::ThreadPost can only acquire thread affinity; "
                    "another key is required to acquire access to the target. Put access keys first.");
        }

        template<class Source, class Target>
        static constexpr auto& acquireAccess(Source&, Target& target)
        {
            constexpr auto requiredThreadId = ContextOf<Target>::Info::RequiredThreadId;
            // Would be a ARC library bug if calling acquireAccess despite target accepting any thread id
            static_assert(requiredThreadId != ThreadEnvironment::AnyThreadId);
            constexpr auto currentThreadId = detail::getCurrentStaticThread<Source>();

            using Environment = Source::Environment;
            using Env = Environment::template InsertOrReplace<ThreadEnvironment::WithId<requiredThreadId>>;
            using WithEnv = arc::TransferEnv<Env, Target>;
            static_assert(WithEnv::Environment::template Get<ThreadEnvironment::Tag>::ThreadId == requiredThreadId);
            return detail::downCast<Interface<WithEnv, currentThreadId>>(target);
        }

        // To be overridden in Poster
        template<std::size_t CurrentThreadId, std::size_t RequiredThreadId>
        static constexpr void post(std::size_t requiredThreadId, auto&& task) = delete;
    };

} // namespace key

} // namespace arc


#endif // INCLUDE_ARC_THREAD_HPP
