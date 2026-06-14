#ifndef INCLUDE_ARC_NODES_COLLECTION_HPP
#define INCLUDE_ARC_NODES_COLLECTION_HPP

#include "arc/nodes/adapt.hpp"
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
#include <algorithm>
#include <iterator>
#include <memory>
#include <ranges>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include <vector>
#endif

namespace arc {

namespace detail {

    struct AlwaysTruePred
    {
        constexpr std::true_type operator()(auto&&...) const { return {}; }
    };

    template<class Node>
    struct StaticHandle
    {
        StaticHandle() = delete;
        auto operator<=>(StaticHandle const&) const = default;
    private:
        friend Node;
        constexpr explicit StaticHandle(std::size_t index) : index(index) {}
        std::size_t index;
    };

    template<class Node, class Element>
    struct DynamicHandle;

    template<class Node, class Element>
    struct WeakDynamicHandle
    {
        WeakDynamicHandle() = default;

        auto lock() const noexcept { return DynamicHandle<Node, Element>(ptr); }
        auto operator<=>(WeakDynamicHandle const&) const = default;
        void reset() noexcept { ptr.reset(); }

    private:
        friend Node;
        friend struct DynamicHandle<Node, Element>;
        constexpr explicit WeakDynamicHandle(std::weak_ptr<Element> element) : ptr(element.lock()) {}
        std::weak_ptr<Element> ptr{};
    };

    template<class Node, class Element>
    struct DynamicHandle
    {
        DynamicHandle() = default;

        auto operator<=>(DynamicHandle const&) const = default;
        constexpr operator bool() const noexcept { return static_cast<bool>(ptr); }
        constexpr bool empty() const noexcept { return !ptr; }

        constexpr auto toWeak() const noexcept { return WeakDynamicHandle<Node, Element>(ptr); }
        constexpr operator WeakDynamicHandle<Node, Element>() const noexcept { return toWeak(); }
        void reset() noexcept { ptr.reset(); }

    private:
        friend Node;
        friend struct WeakDynamicHandle<Node, Element>;
        constexpr explicit DynamicHandle(std::shared_ptr<Element> element) : ptr(std::move(element)) {}
        constexpr explicit DynamicHandle(std::weak_ptr<Element> element) : ptr(element.lock()) {}
        std::shared_ptr<Element> ptr{};
    };

    template<class Node, class Element, bool IsDynamic>
    auto getWeakNodeHandle() -> WeakDynamicHandle<Node, Element>;
    template<class Node, class Element, bool IsDynamic>
    requires (not IsDynamic)
    auto getWeakNodeHandle() -> StaticHandle<Node>;
    template<class Node, class Element, bool IsDynamic>
    auto getStrongNodeHandle() -> DynamicHandle<Node, Element>;
    template<class Node, class Element, bool IsDynamic>
    requires (not IsDynamic)
    auto getStrongNodeHandle() -> StaticHandle<Node>;

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

ARC_MODULE_EXPORT
template<IsNodeHandle NodeHandle, class ID>
using DynamicCollection = Collection<NodeHandle, ID, true>;

ARC_MODULE_EXPORT
template<IsNodeHandle NodeHandle, class ID>
using StaticCollection = Collection<NodeHandle, ID, false>;

ARC_MODULE_EXPORT
template<IsNodeHandle NodeHandle, class ID_, bool Dynamic/* = false*/>
struct Collection
{
    using ID = ID_;

    template<class Context>
    class Node : public arc::Node
    {
        struct Element;
        struct ElementContext;

        using WeakHandle = decltype(detail::getWeakNodeHandle<Node, Element, Dynamic>());
        using StrongHandle = decltype(detail::getStrongNodeHandle<Node, Element, Dynamic>());

        struct ControlTypes
        {
            using WeakHandle = Node::WeakHandle;
            using StrongHandle = Node::StrongHandle;
            using ID = ID_;
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
                return std::as_const(collection->elements)
                    | std::views::filter(
                        [=, this](auto const& el) -> bool
                        {
                            if (&el == this)
                                return false;
                            auto const peer = elToNodeMemPtr.getMemberFromClass(el).asTrait(trait::peer);
                            if (not peer.isPeerId(id))
                                return false;
                            auto const& instance = detail::downCast<NodeState>(detail::upCast<CallerNode>(elToNodeMemPtr.getMemberFromClass(*this)));
                            return peer.isPeerInstance(instance);
                        })
                    | std::views::transform(
                        [=](auto const& el) -> NodeState const&
                        {
                            return detail::downCast<NodeState>(detail::upCast<CallerNode>(elToNodeMemPtr.getMemberFromClass(el)));
                        });
            }

            constexpr StrongHandle getElementHandle() const
            {
                if constexpr (Dynamic)
                {
                    // const_cast is safe since handle is opaque and ControlView::fromHandle enforces const correctness at point of use
                    return StrongHandle(const_cast<Element*>(this)->shared_from_this());
                }
                else
                {
                    auto const index = this - std::to_address(collection->elements.begin());
                    return StrongHandle(index);
                }
            }
        };

        struct ElementContext : Context
        {
            template<IsTrait Trait>
            requires detail::HasLink<Context, Trait>
            static constexpr auto getNode(auto& node, Trait)
            {
                return Context{}.getNode(getCollection(node), detail::ResolveLinkTrait<Context, Trait>{});
            }

            template<IsGlobalTrait GlobalTrait>
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

        using Item = std::conditional_t<Dynamic, std::shared_ptr<Element>, Element>;
        std::vector<Item> elements;
        std::vector<ID> ids;

    protected:
        // Adding elements to this collection must be managed, so this is protected
        constexpr std::pair<Element*, bool> addImpl(bool unique, ID const& id, auto&&... args)
        {
            // Nodes must not be invalidated by insertions, so the vector must not be resized.
            if constexpr (not Dynamic)
                if (elements.capacity() == elements.size())
                    throw std::length_error("Collection capacity exceeded");
            if (auto item = getItemById(id))
            {
                if (unique)
                    throw std::invalid_argument("ID already exists in collection");
                else
                    return {&*item, false};
            }
            ids.push_back(id);
            if constexpr (Dynamic)
                return {elements.emplace_back(std::make_shared<Element>(id, this, ARC_FWD(args)...)).get(), true};
            else
                return {std::addressof(elements.emplace_back(id, this, ARC_FWD(args)...)), true};
        }

        [[nodiscard]] constexpr auto getItemById(this auto& self, ID const& id)
        {
            auto const it = std::find(self.ids.begin(), self.ids.end(), id);
            if constexpr (Dynamic)
                return it != self.ids.end()
                    ? self.elements[std::distance(self.ids.begin(), it)]
                    : nullptr;
            else
                return it != self.ids.end()
                    ? std::addressof(self.elements[std::distance(self.ids.begin(), it)])
                    : nullptr;
        }

        [[nodiscard]] constexpr decltype(auto) elementsView(this auto& self)
        {
            if constexpr (Dynamic)
                return self.elements | std::views::transform([](auto const& el) -> Element& { return *el; });
            else
                return self.elements;
        }

        template<class Trait>
        struct ControlView;

        template<class Types>
        static auto finaliseTypes(auto const&...) -> Types;
        template<class Types>
        static auto finaliseTypes(key::Collection const&, auto const&...) -> ControlTypes;

    public:
        constexpr explicit Node(std::size_t capacity, auto adder)
        {
            elements.reserve(capacity);
            ids.reserve(capacity);
            adder([this](ID const& id, auto&&... args) { this->addImpl(true, id, ARC_FWD(args)...); });
        }

        Node() requires Dynamic = default;

        constexpr Node(Node const& other)
            : elements(other.elements)
            , ids(other.ids)
        {
            for (auto& element : elementsView())
            {
                element.collection = this;
                element.globalNode.set(this);
            }
        }
        constexpr Node(Node&& other)
            : elements(std::move(other.elements))
            , ids(std::move(other.ids))
        {
            for (auto& element : elementsView())
            {
                element.collection = this;
                element.globalNode.set(this);
            }
        }

        using Traits = arc::TraitsTemplate<TraitsTemplate>;

        template<class Source, class Self, class Types, class... Keys>
        using FinaliseTypes = decltype(finaliseTypes<Types>(std::declval<Keys const&>()...));

        constexpr void visit(this auto& self, auto&& visitor)
        {
            for (auto& el : self.elementsView())
                el.node.visit(visitor);
        }

        [[nodiscard]] constexpr bool contains(ID const& id) const
        {
            return std::ranges::contains(ids, id);
        }

        [[nodiscard]] constexpr auto* getId(this auto& self, ID const& id)
        {
            auto item = self.getItemById(id);
            return item ? std::addressof(item->node) : nullptr;
        }

        // Throws if ID does not exist in collection
        [[nodiscard]] constexpr auto& atId(this auto& self, ID const& id)
        {
            auto item = self.getItemById(id);
            if (item == nullptr) [[unlikely]]
                throw std::out_of_range("Element with given ID does not exist in collection");
            return item->node;
        }

        constexpr ElementNode& insert(ID const& id, auto&&... args) requires Dynamic
        {
            return addImpl(true, id, ARC_FWD(args)...).first->node;
        }

        constexpr std::pair<ElementNode&, bool> tryInsert(ID const& id, auto&&... args) requires Dynamic
        {
            auto [element, inserted] = addImpl(false, id, ARC_FWD(args)...);
            return {element->node, inserted};
        }

        template<class Self, std::invocable<ID, StrongHandle, detail::ConstLike<Self, ElementNode>&> F>
        constexpr void forEach(this Self& self, F&& func)
        {
            for (auto& el : self.elementsView())
                func(el.id, el.getElementHandle(), el.node);
        }

        template<class Self, std::predicate<ID, StrongHandle, detail::ConstLike<Self, ElementNode>&> F>
        constexpr void removeIf(this Self& self, F&& func) requires Dynamic
        {
            auto itEl = self.elements.begin();
            auto itId = self.ids.begin();
            for (; itEl != self.elements.end();)
            {
                if (func((*itEl)->id, (*itEl)->getElementHandle(), (*itEl)->node))
                {
                    itEl = self.elements.erase(itEl);
                    itId = self.ids.erase(itId);
                }
                else
                {
                    ++itEl;
                    ++itId;
                }
            }
        }

        constexpr bool remove(ID const& id) requires Dynamic
        {
            static_assert(not ContextHasGlobalTrait<Context, trait::Scheduler>, "Not supported yet");

            auto const it = std::find(ids.begin(), ids.end(), id);
            if (it == ids.end())
                return false;
            auto const index = std::distance(ids.begin(), it);
            ids.erase(it);
            elements.erase(elements.begin() + index);
            return true;
        }
    };
};

template<IsNodeHandle NodeHandle, class ID, bool Dynamic>
template<class Context>
template<class Trait>
struct Collection<NodeHandle, ID, Dynamic>::Node<Context>::AsTrait : Node
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
        if constexpr (Dynamic)
        {
            auto& element = key.id.ptr->node;
            auto target = element.asTrait(detail::AsRef{}, Trait{});
            return target.ptr->finalise(source, keys...);
        }
        else
        {
            auto& element = self.elements[key.id.index].node;
            auto target = element.asTrait(detail::AsRef{}, Trait{});
            return target.ptr->finalise(source, keys...);
        }
    }

    template<class Self>
    constexpr auto finalise(this Self& self, auto& source, key::Collection const&, auto const&... keys)
    {
        static_assert(Dynamic, "Control view is only available for dynamic collections");
        auto& controlView = detail::downCast<ControlView<Trait>>(detail::upCast<Node>(self));
        // Don't consume the key, as we need to consume it for each element
        return controlView.finalise(source, keys...);
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
                for (auto& el : self.elementsView())
                {
                    if (key.pred(std::as_const(el.id)))
                    {
                        auto target = el.node.asTrait(detail::AsRef{}, Trait{});
                        target.ptr->finalise(self, ks...)->impl(args...);
                    }
                }
            },
        keys);
    }
};

template<IsNodeHandle NodeHandle, class ID, bool Dynamic>
template<class Context>
template<class Trait>
struct Collection<NodeHandle, ID, Dynamic>::Node<Context>::ControlView : Node, trait::DynamicCollectionControl::Meta::Impl
{
    constexpr WeakHandle getWeakHandle(ID const& id) const
    {
        return WeakHandle(this->getItemById(id));
    }
    constexpr StrongHandle getStrongHandle(ID const& id) const
    {
        return StrongHandle(this->getItemById(id));
    }
    static constexpr ID getId(StrongHandle const& handle)
    {
        return handle.ptr->id;
    }

    template<class Self>
    constexpr auto fromId(this Self& self, ID const& id)
    {
        return withEnv<typename Self::Environment>(self.atId(id)).asTrait(Trait{});
    }
    template<class Self>
    constexpr auto fromHandle(this Self&, StrongHandle const& handle)
    {
        return withEnv<typename Self::Environment>(detail::constLike<Self>(handle.ptr->node)).asTrait(Trait{});
    }

    constexpr bool contains(ID const& id) const
    {
        return Node::contains(id);
    }
    constexpr bool contains(StrongHandle const& handle) const
    {
        return handle and Node::contains(handle.ptr->id);
    }

    constexpr std::pair<StrongHandle, bool> tryInsert(ID const& id, auto&&... args)
    {
        static_assert(Dynamic, "tryInsert is only available for dynamic collections");
        auto [element, inserted] = addImpl(false, id, ARC_FWD(args)...);
        return {StrongHandle(element->shared_from_this()), inserted};
    }

    constexpr bool remove(ID const& id)
    {
        static_assert(Dynamic, "remove is only available for dynamic collections");
        return Node::remove(id);
    }
    constexpr bool remove(StrongHandle const& handle)
    {
        static_assert(Dynamic, "remove is only available for dynamic collections");
        return handle.ptr and Node::remove(handle.ptr->id);
    }

    template<class Self>
    constexpr void forEach(this Self& self, auto&& func)
    {
        for (auto& el : self.elements)
            func(el->id, el->getElementHandle(), withEnv<typename Self::Environment>(el->node).asTrait(Trait{}));
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

namespace node {
    ARC_MODULE_EXPORT
    using arc::Collection;
    ARC_MODULE_EXPORT
    using arc::DynamicCollection;
    ARC_MODULE_EXPORT
    using arc::StaticCollection;
}

} // namespace arc

#endif // INCLUDE_ARC_NODES_COLLECTION_HPP
