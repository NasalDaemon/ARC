module;

#if !ARC_IMPORT_STD
#include "arc/arc.hpp"
#endif

export module arc;

#if ARC_IMPORT_STD
export import std;
#define ARC_MODULE_EXPORT export
#include "arc/arc.hpp"
#else

export using ::operator new;

export namespace arc {
    // adapt.hpp
    using arc::Adapt;
    using arc::adapt;
    // alias.hpp
    using arc::Alias;
    using arc::makeAlias;
    // args.hpp
    using arc::Args;
    using arc::args;
    using arc::IsArgs;
    using arc::IsArgsOf;
    // box.hpp
    using arc::BoxWithRoot;
    using arc::boxWithRoot;
    using arc::Box;
    using arc::box;
    // detail/cast.hpp
    namespace detail {
        using detail::memberPtr;
    }
    // circular_buffer.hpp
    using arc::CircularBuffer;
    // cluster.hpp
    using arc::Cluster;
    using arc::IsRootCluster;
    using arc::IsCluster;
    using arc::Domain;
    using arc::IsDomain;
    using arc::IsRootDomain;
    using arc::DomainParams;
    // collection.hpp
    namespace key {
        using key::Element;
        using key::Elements;
        using key::allElements;
    }
    using arc::Collection;
    // combine.hpp
    using arc::Combine;
    // compiler.hpp
    using arc::Version;
    using arc::Compiler;
    using arc::isClang;
    using arc::isGcc;
    using arc::isGnu;
    using arc::isMsvc;
    using arc::compiler;
    using arc::clang;
    using arc::gcc;
    using arc::msvc;
    // context_fwd.hpp
    using arc::Context;
    using arc::IsContext;
    using arc::IsSameContext;
    using arc::IsElementContext;
    using arc::ContextParameterOf;
    using arc::ContextOf;
    using arc::HasContext;
    using arc::ContextToNode;
    using arc::ContextToNodeState;
    using arc::NullContext;
    using arc::RootContext;
    using arc::IsRootContext;
    using arc::HasContext;
    using arc::InlineContext;
    // context.hpp
    using arc::ContextHasTrait;
    using arc::ContextHasTraitPred;
    // count.hpp
    using arc::nodeCount;
    using arc::IsUnary;
    // depends.hpp
    using arc::Depends;
    // detached.hpp
    using arc::DetachedInterface;
    using arc::IsDetachedInterface;
    using arc::DetachedImpl;
    using arc::IsDetachedImpl;
    using arc::HasDetachedContext;
    // defer.hpp
    using arc::Defer;
    // traits/dynamic_node.hxx
    using arc::IsDynamicContext;
    namespace trait {
        using trait::DynamicNode;
        using trait::dynamicNode;
    }
    // empty_types.hpp
    using arc::EmptyTypes;
    using arc::IsStateless;
    // ensure.hpp
    using arc::Ensure;
    namespace pred {
        using pred::Unary;
        using pred::NonUnary;
        using pred::Stateless;
        using pred::HasDepends;
    }
    // environment.hpp
    using arc::Environment;
    using arc::EnvironmentOverlay;
    using arc::EnvironmentPart;
    using arc::mergeEnvParts;
    using arc::withEnv;
    using arc::TransferEnv;
    // factory.hpp
    using arc::WithFactory;
    using arc::withFactory;
    using arc::Constructor;
    using arc::Emplace;
    // function.hpp
    using arc::StatelessInvocable;
    using arc::Function;
    using arc::FunctionPolicy;
    // finalise.hpp
    using arc::finalise;
    // global_context.hpp
    using arc::ContextHasGlobal;
    using arc::ContextHasGlobalTrait;
    // global_graph.hpp
    using arc::GraphWithGlobal;
    // global_trait.hpp
    using arc::Global;
    using arc::IsGlobalTrait;
    using arc::IsNonGlobalTrait;
    using arc::global;
    // graph.hpp
    using arc::InlineGraph;
    using arc::Graph;
    using arc::constructGraph;
    // key.hpp
    namespace key {
        using key::Default;
        using key::IsKey;
        using key::Trait;
    }
    // lazy.hpp
    using arc::Lazy;
    // link.hpp
    using arc::CanGetNode;
    using arc::HasTrait;
    using arc::ResolvedLink;
    using arc::IsResolvedLink;
    // map_info.hpp
    using arc::IsInfoMapper;
    using arc::MapInfo;
    // mock.hpp
    namespace test {
        using test::MockDefault;
        using test::MockParams;
        using test::ArgsTuple;
        using test::argsTuple;
        using test::Mock;
    }
    // narrow.hpp
    using arc::Narrow;
    // node_fwd.hpp
    using arc::Node;
    using arc::IsNode;
    using arc::IsNodeWrapper;
    using arc::WrapNode;
    using arc::IsWrappedImpl;
    using arc::IsNodeHandle;
    using arc::ToNodeWrapper;
    using arc::InlineNode;
    using arc::NodeHasDepends;
    using arc::NodeDependencyListed;
    using arc::NodeDependenciesSatisfied;
    // no_trait.hpp
    using arc::NoTrait;
    using arc::noTrait;
    using arc::NullTrait;
    using arc::IsNoTrait;
    using arc::NoTraits;
    // peer_node.hpp
    using arc::PeerNode;
    using arc::PeerDetached;
    using arc::PeerDetachedOpen;
    // traits/peer.hxx
    namespace trait {
        using trait::Peer;
        using trait::peer;
    }
    // repeater.hpp
    using arc::Repeater;
    using arc::RepeaterTrait;
    // resolve.hpp
    using arc::CanResolve;
    using arc::ResolveTypes;
    using arc::ResolveRoot;
    using arc::ResolveInfo;
    // traits/scheduler.hpp
    using arc::TerminateSchedulerThreadException;
    using arc::StopSchedulerException;
    namespace trait {
        using trait::Scheduler;
        using trait::scheduler;
    }
    // traits/spy.hpp
    namespace trait {
        using arc::trait::SpyOnly;
        using arc::trait::spyOnly;
        using arc::trait::Spy;
        using arc::trait::spy;
    }
    // test.hpp
    namespace test {
        using test::IsTestContext;
        using test::Local;
        using test::local;
        using test::MockTrait;
        using test::MockKey;
        using test::TestOnlyNode;
        using test::Cluster;
        using test::Graph;
        using test::GraphWithGlobal;
    }
    // thread.hpp
    using arc::ThreadEnvironment;
    using arc::OnThread;
    using arc::AnyThread;
    using arc::withThread;
    using arc::OnDynThread;
    namespace key {
        using key::ThreadPost;
        using key::DynThreadAssert;
    }
    // trait_view.hpp
    using arc::IsTraitView;
    using arc::IsTraitViewOf;
    using arc::AsFunctor;
    using arc::asFunctor;
    using arc::AutoCompleteTraitView;
    using arc::TraitView;
    using arc::makeTraitView;
    // trait.hpp
    using arc::AdlTag;
    using arc::Trait;
    using arc::IsTrait;
    using arc::UnconstrainedTrait;
    using arc::TraitExpects;
    using arc::TraitCanProvide;
    using arc::MatchesTrait;
    using arc::ExactlyMatchesTrait;
    using arc::Implements;
    using arc::IsMethodOf;
    using arc::TraitOf;
    using arc::JoinedTrait;
    using arc::AltTrait;
    // traits_fwd.hpp
    using arc::ResolvedTrait;
    using arc::IsResolvedTrait;
    using arc::Traits;
    using arc::TraitsOpen;
    using arc::TraitsTemplate;
    using arc::TraitsFrom;
    // type_id.hpp
    using arc::TypeId;
    using arc::typeId;
    // type_name.hpp
    using arc::typeName;
    // union.hpp
    using arc::Union;
    using arc::withIndex;
    // virtual.hpp
    using arc::IDestructible;
    using arc::KeepAlive;
    using arc::IsInterface;
    using arc::IsVirtualContext;
    using arc::INode;
    using arc::Virtual;
}

#endif
