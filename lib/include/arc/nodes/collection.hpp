#ifndef INCLUDE_ARC_NODES_COLLECTION_HPP
#define INCLUDE_ARC_NODES_COLLECTION_HPP

#include "arc/nodes/adapt.hpp"
#include "arc/nodes/collection_storage.hpp"
#include "arc/detail/as_ref.hpp"
#include "arc/detail/cast.hpp"

#include "arc/context_fwd.hpp"
#include "arc/traits/collection_control.hxx"
#include "arc/traits/scheduler.hpp"
#include "arc/detail/compress.hpp"
#include "arc/detail/concepts.hpp"
#include "arc/empty_types.hpp"
#include "arc/environment.hpp"
#include "arc/finalise.hpp"
#include "arc/global_context.hpp"
#include "arc/global_trait.hpp"
#include "arc/key.hpp"
#include "arc/macros.hpp"
#include "arc/node.hpp"
#include "arc/node_fwd.hpp"
#include "arc/traits/peer.hxx"
#include "arc/traits.hpp"

#if !ARC_IMPORT_STD
#include <memory>
#include <ranges>
#include <stdexcept>
#include <type_traits>
#include <utility>
#endif

namespace arc {

namespace detail {

    struct AlwaysTruePred
    {
        constexpr std::true_type operator()(auto&&...) const { return {}; }
    };

} // namespace detail

namespace key {

    ARC_MODULE_EXPORT
    template<class ID>
    struct Element : key::Default
    {
        explicit Element(ID id) : id(id) {}
        ID id;
    };

    template<class ID>
    Element(ID) -> Element<ID>;

    ARC_MODULE_EXPORT
    template<class Predicate>
    struct Elements : key::Default
    {
        template<class T>
        using Trait = detail::DuckTrait<T>;

        Elements() = default;
        explicit Elements(Predicate pred) : pred(std::move(pred)) {}

        [[no_unique_address]] Predicate pred{};
    };

    template<class Pred>
    Elements(Pred) -> Elements<Pred>;

    ARC_MODULE_EXPORT
    inline constexpr Elements<detail::AlwaysTruePred> allElements{};

    // Offers a control view over a dynamic collection, with trait arc::trait::CollectionControl.
    // CollectionControl methods: getWeakHandle, getStrongHandle, exists, tryInsert, remove
    ARC_MODULE_EXPORT
    struct Collection : key::Default
    {
        template<class T>
        using Trait = arc::trait::DynamicCollectionControl;
    };

    ARC_MODULE_EXPORT
    inline constexpr Collection collection{};

} // namespace key

// Collection of identically-typed nodes; Policy selects element lifetime and storage
// layout (see arc::CollectionPolicy). Prefer the Dynamic/Static[Map|Index] aliases below.
ARC_MODULE_EXPORT
template<IsNodeHandle NodeHandle, class ID_, class Policy>
struct Collection
{
    using ID = ID_;

    static constexpr bool Dynamic = Policy::dynamic;

    template<class Context>
    class Node : public arc::Node
    {
        struct Element;
        struct ElementContext;

        // Memory layout and element/id lookup are delegated to the storage policy,
        // so the most optimal layout can be selected per collection.
        using Storage = Policy::template Storage<Element, ID_>;

        // Either ID, or a placeholder (e.g. arc::AutoId) when the storage assigns ids itself
        using InsertId = Storage::InsertId;

        using WeakHandle = Storage::WeakHandle;
        using StrongHandle = Storage::StrongHandle;

        struct ControlTypes
        {
            using WeakHandle = Node::WeakHandle;
            using StrongHandle = Node::StrongHandle;
            using ID = ID_;
            // Insertion id: the caller-chosen ID for map storages, or arc::AutoId
            // for index storages that assign the id themselves.
            using InsertId = Node::InsertId;
        };

        using ElementNode = detail::ToNodeState<typename ToNodeWrapper<NodeHandle>::template Node<detail::CompressContext<ElementContext>>>;

        friend consteval ElementNode getNodeTypeToInstantiate(arc::AdlTag<Node>) { return std::declval<ElementNode>(); }

        struct Element : std::conditional_t<Dynamic, std::enable_shared_from_this<Element>, detail::Void>
        {
            ID id;
            Node* collection;
            [[no_unique_address]] detail::GlobalNodePtr<Context> globalNode;
            [[no_unique_address]] ElementNode node;

            explicit Element(ID const& id, Node* collection, auto&&... args)
                : id(id), collection(collection), globalNode(collection), node(ARC_FWD(args)...)
            {}

            template<class Caller, class Target>
            constexpr auto getPeers(detail::MemberPtr<Element, Target> elToNodeMemPtr) const
            {
                using CallerNode = UnderlyingNode<Caller>;
                using TargetNode = UnderlyingNode<Target>;
                // Remove dynamic environment components from Caller, as the peers are independent instances
                using Environment = Caller::Environment::template RemoveDynamic<>;
                using NodeState = TransferEnv<Environment, arc::ContextToNodeState<detail::Decompress<ContextOf<Caller>>>>;
                static_assert(std::is_same_v<CallerNode, TargetNode>);
                return collection->elementsView()
                    | std::views::transform([](auto const& item) { return std::to_address(item); })
                    | std::views::filter(
                        [=, this](auto const* el) -> bool
                        {
                            if (el == this)
                                return false;
                            auto const peer = elToNodeMemPtr.getMemberFromClass(*el).asTrait(arc::trait::peer);
                            if (not peer.isPeerId(id))
                                return false;
                            auto const& instance = detail::downCast<NodeState>(detail::upCast<CallerNode>(elToNodeMemPtr.getMemberFromClass(*this)));
                            return peer.isPeerInstance(instance);
                        })
                    | std::views::transform(
                        [=](auto const* el) -> NodeState const&
                        {
                            return detail::downCast<NodeState>(detail::upCast<CallerNode>(elToNodeMemPtr.getMemberFromClass(*el)));
                        });
            }

            constexpr StrongHandle getElementHandle() const
            {
                return collection->store.handleFor(this);
            }
        };

        struct ElementContext : Context
        {
            template<IsTrait Trait>
            requires detail::LocallyResolvable<Context, Trait>
            static constexpr auto getNode(auto& node, Trait)
            {
                return Context{}.getNode(getCollection(node), detail::ResolveLinkTrait<Context, Trait>{});
            }

            template<IsGlobalTrait GlobalTrait>
            requires ContextHasGlobal<Context>
            static constexpr auto getNode(auto& node, GlobalTrait)
            {
                detail::assertContextHasGlobalTrait<ElementContext, GlobalTrait>();
                return getGlobalNode(node).asTrait(detail::AsRef{}, typename GlobalTrait::Trait{});
            }

            template<class N>
            static constexpr auto& getGlobalNode(N& node)
            {
                // Global node pointer is cached in Element to avoid multiple lookups
                return std::forward_like<N&>(*getElement(node).globalNode.get());
            }

            template<IsContext Parent>
            constexpr auto getParentMemPtr()
            {
                static_assert(std::is_same_v<Parent, ElementContext>, "arc::Collection does not have a stable member pointer relative to its parent node");
                return ARC_MEM_PTR(Element, node);
            }

            template<IsContext Parent>
            constexpr auto& getParentNode(auto& node)
            {
                if constexpr (std::is_same_v<detail::Decompress<Parent>, ElementContext>)
                    return node;
                else
                    return Context{}.template getParentNode<Parent>(getCollection(node));
            }

            using IdType = ID;
            using CollectionContext = Context;

            struct Info : Context::Info
            {
                using ElementContext = Node::ElementContext;
            };

        private:
            static constexpr auto& getElement(auto& node)
            {
                auto memPtr = ARC_MEM_PTR(Element, node);
                return memPtr.getClassFromMember(node);
            }
            template<class InnerNode>
            static constexpr auto& getCollection(InnerNode& node)
            {
                return std::forward_like<InnerNode&>(*getElement(node).collection);
            }
        };

        template<class>
        struct AsTrait;

        template<class Trait>
        requires HasTrait<ElementNode, Trait>
        using TraitsTemplate = arc::ResolvedTrait<AsTrait<Trait>, detail::ResolveTypesOfNode<ElementNode, Trait>>;

    protected:
        Storage store;

        // Adding elements to this collection must be managed, so this is protected
        constexpr std::pair<Element*, bool> addImpl(bool unique, InsertId const& id, auto&&... args)
        {
            // Checked here rather than at class scope, as Element must be complete
            if constexpr (Dynamic)
                static_assert(IsDynamicCollectionStorage<Storage, Element, ID>);
            else
                static_assert(IsCollectionStorage<Storage, Element, ID>);

            // Nodes must not be invalidated by insertions, so the store must not relocate elements.
            store.ensureSpareCapacity();
            // Storage-assigned ids cannot collide, so only caller-chosen ids are checked for duplicates
            if constexpr (std::is_same_v<InsertId, ID>)
            {
                if (auto* element = store.findById(id))
                {
                    if (unique)
                        throw std::invalid_argument("ID already exists in collection");
                    else
                        return {element, false};
                }
            }
            Element* element = store.emplace(id, this, ARC_FWD(args)...);
            return {element, element != nullptr};
        }

        [[nodiscard]] constexpr decltype(auto) elementsView(this auto& self)
        {
            return self.store.view();
        }

        template<class Trait>
        struct ControlView;

        template<class Types>
        static auto finaliseTypes(auto const&...) -> Types;
        template<class Types>
        static auto finaliseTypes(key::Collection const&, auto const&...) -> ControlTypes;

    private:
        constexpr void rebindElements()
        {
            for (auto element : elementsView())
            {
                element->collection = this;
                element->globalNode.set(this);
            }
        }

    public:
        constexpr explicit Node(std::size_t capacity, auto adder)
        {
            store.reserve(capacity);
            adder([this](InsertId const& id, auto&&... args)
            {
                if (this->addImpl(true, id, ARC_FWD(args)...).first == nullptr) [[unlikely]]
                    throw std::length_error("Collection storage failed to insert element");
            });
        }

        Node() requires Dynamic = default;

        constexpr Node(Node const& other)
            : store(other.store)
        {
            rebindElements();
        }
        constexpr Node(Node&& other)
            : store(std::move(other.store))
        {
            rebindElements();
        }

        using Traits = arc::TraitsTemplate<TraitsTemplate>;

        template<class Source, class Self, class Types, class... Keys>
        using FinaliseTypes = decltype(finaliseTypes<Types>(std::declval<Keys const&>()...));

        constexpr void visit(this auto& self, auto&& visitor)
        {
            for (auto el : self.elementsView())
                el->node.visit(visitor);
        }

        [[nodiscard]] constexpr bool contains(ID const& id) const
        {
            return store.contains(id);
        }

        [[nodiscard]] constexpr auto* getId(this auto& self, ID const& id)
        {
            auto* item = self.store.findById(id);
            return item ? std::addressof(item->node) : nullptr;
        }

        // Throws if ID does not exist in collection
        [[nodiscard]] constexpr auto& atId(this auto& self, ID const& id)
        {
            auto* item = self.store.findById(id);
            if (item == nullptr) [[unlikely]]
                throw std::out_of_range("Element with given ID does not exist in collection");
            return item->node;
        }

        template<class Self>
        [[nodiscard]] constexpr auto& atHandle(this Self& self, StrongHandle const& handle)
        {
            return detail::constLike<Self>(self.store.elementOf(handle)->node);
        }

        [[nodiscard]] constexpr ElementNode* insert(InsertId const& id, auto&&... args) requires Dynamic
        {
            static_assert(not ContextHasGlobalTrait<Context, arc::trait::Scheduler>, "Not supported yet");

            Element* el = addImpl(true, id, ARC_FWD(args)...).first;
            if (el)
                el->node.visit(detail::OnGraphConstructedVisitor{});
            return el ? std::addressof(el->node) : nullptr;
        }

        [[nodiscard]] constexpr std::pair<ElementNode*, bool> tryInsert(InsertId const& id, auto&&... args) requires Dynamic
        {
            static_assert(not ContextHasGlobalTrait<Context, arc::trait::Scheduler>, "Not supported yet");

            auto [element, inserted] = addImpl(false, id, ARC_FWD(args)...);
            if (element && inserted)
                element->node.visit(detail::OnGraphConstructedVisitor{});
            return {element ? std::addressof(element->node) : nullptr, inserted};
        }

        template<class Self, std::invocable<ID, StrongHandle, detail::ConstLike<Self, ElementNode>&> F>
        constexpr void forEach(this Self& self, F&& func)
        {
            for (auto el : self.elementsView())
                func(el->id, el->getElementHandle(), el->node);
        }

        template<class Self, std::predicate<ID, StrongHandle, detail::ConstLike<Self, ElementNode>&> F>
        constexpr void removeIf(this Self& self, F&& func) requires Dynamic
        {
            static_assert(not ContextHasGlobalTrait<Context, arc::trait::Scheduler>, "Not supported yet");

            self.store.eraseIf(
                [&](detail::ConstLike<Self, Element>& el)
                {
                    return func(el.id, el.getElementHandle(), el.node);
                });
        }

        constexpr bool remove(ID const& id) requires Dynamic
        {
            static_assert(not ContextHasGlobalTrait<Context, arc::trait::Scheduler>, "Not supported yet");

            return store.erase(id);
        }
    };
};

template<IsNodeHandle NodeHandle, class ID, class Policy>
template<class Context>
template<class Trait>
struct Collection<NodeHandle, ID, Policy>::Node<Context>::AsTrait : Node
{
    template<class Self>
    constexpr auto finalise(this Self& self, auto& source, key::Element<ID> const& key, auto const&... keys)
    {
        auto* const element = self.getId(key.id);
        if (element == nullptr) [[unlikely]]
            throw std::out_of_range("Element with given ID does not exist in collection");
        auto target = element->asTrait(detail::AsRef{}, Trait{});
        return target.ptr->finalise(source, keys...);
    }

    template<class Self>
    constexpr auto finalise(this Self& self, auto& source, key::Element<StrongHandle> const& key, auto const&... keys)
    {
        auto& element = self.store.elementOf(key.id)->node;
        auto target = element.asTrait(detail::AsRef{}, Trait{});
        return target.ptr->finalise(source, keys...);
    }

    template<class Self, class Source>
    constexpr auto finalise(this Self& self, Source& source, key::Collection const&, auto const&... keys)
    {
        auto& controlView = detail::downCast<ControlView<Trait>>(detail::upCast<Node>(self));
        auto& controlViewEnv = mergeEnvParts<SourceNodeEnvironment::WithSourceNode<Source>>(controlView);
        return controlViewEnv.finalise(source, keys...);
    }

    template<class T>
    static constexpr auto finalise(auto&, key::Element<T> const&, auto const&...)
    {
        static_assert(std::is_same_v<T, ID> or std::is_same_v<T, StrongHandle>, "T is not ID or Handle type");
    }

    ARC_INLINE constexpr auto finalise(this auto& self, auto& source, auto const& key, auto const&... keys)
    {
        // Don't consume the key, as we need to consume it for each element
        return arc::finalise<false>(source, self, key, keys...);
    }

    constexpr void implWithKey(this auto& self, key::Default const&, auto const& keys, auto&&... args)
    {
        // Default key should behave like key::allElements
        return self.implWithKey(key::allElements, keys, ARC_FWD(args)...);
    }

    template<class Self, class Pred, class... Args>
    constexpr void implWithKey(this Self& self, key::Elements<Pred> const& key, auto const& keys, Args&&... args)
    {
        std::apply(
            [&](auto const&... ks)
            {
                for (auto el : self.elementsView())
                {
                    if (key.pred(std::as_const(el->id)))
                    {
                        auto target = el->node.asTrait(detail::AsRef{}, Trait{});
                        target.ptr->finalise(self, ks...)->impl(args...);
                    }
                }
            },
        keys);
    }
};

template<IsNodeHandle NodeHandle, class ID, class Policy>
template<class Context>
template<class Trait>
struct Collection<NodeHandle, ID, Policy>::Node<Context>::ControlView : Node, arc::trait::DynamicCollectionControl::Meta::Impl
{
    constexpr WeakHandle getWeakHandle(ID const& id) const
    {
        return this->store.weakHandleFor(id);
    }
    constexpr StrongHandle getStrongHandle(ID const& id) const
    {
        return this->store.strongHandleFor(id);
    }
    static constexpr ID getId(StrongHandle const& handle)
    {
        return Storage::idOf(handle);
    }

    template<class Self>
    constexpr auto fromId(this Self& self, ID const& id)
    {
        return withEnv<typename Self::Environment>(self.atId(id)).asTrait(Trait{});
    }
    template<class Self>
    constexpr auto fromHandle(this Self& self, StrongHandle const& handle)
    {
        return withEnv<typename Self::Environment>(self.atHandle(handle)).asTrait(Trait{});
    }

    constexpr bool contains(ID const& id) const
    {
        return Node::contains(id);
    }
    constexpr bool contains(StrongHandle const& handle) const
    {
        return this->store.contains(handle);
    }

    [[nodiscard]] constexpr std::pair<StrongHandle, bool> tryInsert(InsertId const& id, auto&&... args)
    {
        static_assert(Dynamic, "tryInsert is only available for dynamic collections");
        auto [element, inserted] = addImpl(false, id, ARC_FWD(args)...);
        return {this->store.handleFor(element), inserted};
    }

    constexpr bool remove(ID const& id)
    {
        static_assert(Dynamic, "remove is only available for dynamic collections");
        return Node::remove(id);
    }
    constexpr bool remove(StrongHandle const& handle)
    {
        static_assert(Dynamic, "remove is only available for dynamic collections");
        return this->store.contains(handle) and Node::remove(Storage::idOf(handle));
    }

    template<class Self>
    constexpr void forEach(this Self& self, auto&& func)
    {
        for (auto el : self.elementsView())
            func(el->id, el->getElementHandle(), withEnv<typename Self::Environment>(el->node).asTrait(Trait{}));
    }
    template<class Self>
    constexpr StrongHandle findIf(this Self& self, auto&& func)
    {
        for (auto el : self.elementsView())
        {
            auto handle = el->getElementHandle();
            if (func(el->id, handle, withEnv<typename Self::Environment>(el->node).asTrait(Trait{})))
            {
                return std::move(handle);
            }
        }
        return {};
    }
    template<class Self>
    constexpr void removeIf(this Self& self, auto&& func)
    {
        static_assert(Dynamic, "removeIf is only available for dynamic collections");
        self.Node::removeIf(
            [&](ID const& id, StrongHandle handle, auto& node)
            {
                return func(id, handle, withEnv<typename Self::Environment>(node).asTrait(Trait{}));
            });
    }
};

// Collection of nodes keyed by caller-chosen ids
ARC_MODULE_EXPORT
template<IsNodeHandle NodeHandle, class ID>
using DynamicMap = Collection<NodeHandle, ID, DynamicMapPolicy>;

ARC_MODULE_EXPORT
template<IsNodeHandle NodeHandle, class ID>
using StaticMap = Collection<NodeHandle, ID, StaticMapPolicy>;

// Collection of nodes keyed by storage-assigned indices; insertion takes arc::autoId
ARC_MODULE_EXPORT
template<IsNodeHandle NodeHandle, class ID = std::size_t>
using DynamicIndex = Collection<NodeHandle, ID, DynamicIndexPolicy>;

ARC_MODULE_EXPORT
template<IsNodeHandle NodeHandle, class ID = std::size_t>
using StaticIndex = Collection<NodeHandle, ID, StaticIndexPolicy>;

namespace node {
    ARC_MODULE_EXPORT
    using arc::DynamicMap;
    ARC_MODULE_EXPORT
    using arc::StaticMap;
    ARC_MODULE_EXPORT
    using arc::DynamicIndex;
    ARC_MODULE_EXPORT
    using arc::StaticIndex;
}

} // namespace arc

#endif // INCLUDE_ARC_NODES_COLLECTION_HPP
