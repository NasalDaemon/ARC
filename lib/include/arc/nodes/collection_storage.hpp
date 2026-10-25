#ifndef INCLUDE_ARC_NODES_COLLECTION_STORAGE_HPP
#define INCLUDE_ARC_NODES_COLLECTION_STORAGE_HPP

#include "arc/assert_handlers.hpp"
#include "arc/detail/cast.hpp"
#include "arc/empty_types.hpp"
#include "arc/macros.hpp"
#include "arc/static_assert.hpp"

#if !ARC_IMPORT_STD
#include <algorithm>
#include <concepts>
#include <cstddef>
#include <iterator>
#include <limits>
#include <memory>
#include <ranges>
#include <stdexcept>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>
#endif

namespace arc {

namespace detail {

    // Handle representation (index, shared_ptr, generation key, ...) is a memory
    // layout decision, so handles are owned and constructed by the storage type.
    template<class Storage>
    struct StaticHandle
    {
        StaticHandle() = delete;
        auto operator<=>(StaticHandle const&) const = default;
    private:
        friend Storage;
        constexpr explicit StaticHandle(std::size_t index) : index(index) {}
        std::size_t index;
    };

    template<class Storage, class Element>
    struct DynamicHandle;

    template<class Storage, class Element>
    struct WeakDynamicHandle
    {
        WeakDynamicHandle() = default;

        auto lock() const noexcept { return DynamicHandle<Storage, Element>(ptr); }
        bool empty() const noexcept { return *this == WeakDynamicHandle{}; }

        bool operator==(WeakDynamicHandle const& other) const noexcept
        {
            return not ptr.owner_before(other.ptr) and not other.ptr.owner_before(ptr);
        }
        bool operator==(DynamicHandle<Storage, Element> const& other) const noexcept
        {
            return not ptr.owner_before(other.ptr) and not other.ptr.owner_before(ptr);
        }

        void reset() noexcept { ptr.reset(); }

    private:
        friend Storage;
        friend struct DynamicHandle<Storage, Element>;
        constexpr explicit WeakDynamicHandle(std::weak_ptr<Element> element) : ptr(element) {}
        std::weak_ptr<Element> ptr{};
    };

    template<class Storage, class Element>
    struct DynamicHandle
    {
        DynamicHandle() = default;

        auto operator<=>(DynamicHandle const&) const = default;
        constexpr operator bool() const noexcept { return static_cast<bool>(ptr); }
        constexpr bool empty() const noexcept { return !ptr; }

        constexpr auto toWeak() const noexcept { return WeakDynamicHandle<Storage, Element>(ptr); }
        constexpr operator WeakDynamicHandle<Storage, Element>() const noexcept { return toWeak(); }
        void reset() noexcept { ptr.reset(); }

    private:
        friend Storage;
        friend struct WeakDynamicHandle<Storage, Element>;
        constexpr explicit DynamicHandle(std::shared_ptr<Element> element) : ptr(std::move(element)) {}
        constexpr explicit DynamicHandle(std::weak_ptr<Element> element) : ptr(element.lock()) {}
        std::shared_ptr<Element> ptr{};
    };

} // namespace detail

// Placeholder insertion id for storages that assign element ids automatically
ARC_MODULE_EXPORT
struct AutoId
{
    auto operator<=>(AutoId const&) const = default;
};

ARC_MODULE_EXPORT
inline constexpr AutoId autoId{};

// Interface expected of an arc::Collection storage policy.
// Element must expose a public `id` member of type ID, and on dynamic layouts
// requiring shared ownership it must derive std::enable_shared_from_this.
// Elements are constructed as Element(id, args...), where the id is either the
// caller-provided InsertId, or assigned by the storage when InsertId is arc::AutoId.
// emplace returns nullptr when the storage cannot accommodate the element (e.g. id
// space exhausted); ensureSpareCapacity must throw whenever the next emplace would
// fail for capacity reasons.
ARC_MODULE_EXPORT
template<class S, class Element, class ID>
concept IsCollectionStorage = requires (
    S& s,
    S const& cs,
    ID const& id,
    S::InsertId const& insertId,
    S::StrongHandle const& handle,
    Element const* element)
{
    typename S::InsertId;
    typename S::WeakHandle;
    typename S::StrongHandle;

    // Capacity
    s.reserve(std::size_t{});
    { cs.size() } -> std::convertible_to<std::size_t>;
    { cs.capacity() } -> std::convertible_to<std::size_t>;
    cs.ensureSpareCapacity();

    // Element/id operations
    { cs.contains(id) } -> std::same_as<bool>;
    { cs.contains(handle) } -> std::same_as<bool>;
    { s.findById(id) } -> std::same_as<Element*>;
    { cs.findById(id) } -> std::same_as<Element const*>;
    { s.emplace(insertId) } -> std::same_as<Element*>;

    // Iteration: range of Element pointers (or pointer-like) supporting `el->member`
    { cs.view() } -> std::ranges::input_range;
    { s.view() } -> std::ranges::input_range;

    // Handles
    { cs.handleFor(element) } -> std::same_as<typename S::StrongHandle>;
    cs.elementOf(handle);
    s.elementOf(handle);
};

// Additional operations required of storage used by a dynamic arc::Collection
ARC_MODULE_EXPORT
template<class S, class Element, class ID>
concept IsDynamicCollectionStorage = IsCollectionStorage<S, Element, ID>
    and requires (
        S& s,
        S const& cs,
        ID const& id,
        S::StrongHandle const& handle,
        bool (*pred)(Element const&))
{
    { s.erase(id) } -> std::same_as<bool>;
    s.eraseIf(pred);
    { cs.weakHandleFor(id) } -> std::same_as<typename S::WeakHandle>;
    { cs.strongHandleFor(id) } -> std::same_as<typename S::StrongHandle>;
    { cs.idOf(handle) } -> std::convertible_to<ID>;
};

namespace detail {

    // Handles of storages holding each element in its own shared_ptr; the storage provides findById.
    // Element is unique per collection node, so handles remain distinct types.
    template<class Element, class ID>
    struct DynamicHandles
    {
        using WeakHandle = WeakDynamicHandle<DynamicHandles, Element>;
        using StrongHandle = DynamicHandle<DynamicHandles, Element>;

        [[nodiscard]] constexpr bool contains(this auto const& self, StrongHandle const& handle)
        {
            return handle.ptr and self.findById(handle.ptr->id) == handle.ptr.get();
        }

        [[nodiscard]] static constexpr StrongHandle handleFor(Element const* element)
        {
            return StrongHandle(lockElement(element));
        }
        [[nodiscard]] constexpr WeakHandle weakHandleFor(this auto const& self, ID const& id)
        {
            return WeakHandle(lockElement(self.findById(id)));
        }
        [[nodiscard]] constexpr StrongHandle strongHandleFor(this auto const& self, ID const& id)
        {
            return StrongHandle(lockElement(self.findById(id)));
        }

        [[nodiscard]] static constexpr ID idOf(StrongHandle const& handle)
        {
            return handle.ptr->id;
        }

        template<class Self>
        [[nodiscard]] constexpr ConstLike<Self, Element>* elementOf(this Self&, StrongHandle const& handle)
        {
            return handle.ptr.get();
        }

    private:
        [[nodiscard]] static constexpr std::shared_ptr<Element> lockElement(Element const* element)
        {
            // const_cast is safe since the handle is opaque and consumers enforce const correctness at point of use
            return element ? const_cast<Element*>(element)->shared_from_this() : nullptr;
        }
    };

    // Handles of storages holding elements inline in an `items` vector that never relocates.
    // Element is unique per collection node, so handles remain distinct types.
    template<class Element>
    struct StaticVectorHandles
    {
        using WeakHandle = StaticHandle<StaticVectorHandles>;
        using StrongHandle = StaticHandle<StaticVectorHandles>;

        [[nodiscard]] constexpr bool contains(this auto const& self, StrongHandle const& handle)
        {
            return handle.index < self.items.size();
        }

        [[nodiscard]] constexpr StrongHandle handleFor(this auto const& self, Element const* element)
        {
            return StrongHandle(static_cast<std::size_t>(element - std::to_address(self.items.begin())));
        }

        [[nodiscard]] constexpr auto* elementOf(this auto& self, StrongHandle const& handle)
        {
            return std::addressof(self.items[handle.index]);
        }
    };

    // Common base for vector-backed collection storages, owning the item vector: shared_ptrs
    // in the dynamic layout, inline elements in the static one. The derived storage provides
    // the id scheme: findById, emplace, erase, eraseIf, and any id bookkeeping.
    template<class Element, class ID, bool Dynamic>
    struct VectorStorageBase : std::conditional_t<Dynamic, DynamicHandles<Element, ID>, StaticVectorHandles<Element>>
    {
        friend StaticVectorHandles<Element>;

        constexpr void reserve(std::size_t capacity)
        {
            items.reserve(capacity);
        }

        [[nodiscard]] constexpr std::size_t size() const noexcept { return items.size(); }
        [[nodiscard]] constexpr std::size_t capacity() const noexcept { return items.capacity(); }

        // Inline elements must never relocate, as that would invalidate live node references
        constexpr void ensureSpareCapacity() const
        {
            if constexpr (not Dynamic)
                if (items.capacity() == items.size())
                    throw std::length_error("Collection capacity exceeded");
        }

        // Range of Element pointers (or shared_ptrs) supporting `el->member` access
        [[nodiscard]] constexpr decltype(auto) view(this auto& self)
        {
            if constexpr (Dynamic)
                return (self.items);
            else
                return self.items | std::views::transform([](auto& el) { return std::addressof(el); });
        }

    protected:
        using Item = std::conditional_t<Dynamic, std::shared_ptr<Element>, Element>;

        // Propagates item constness to the element, as shared_ptr is only shallow-const
        template<class Item_>
        static constexpr auto* itemToPtr(Item_& item)
        {
            if constexpr (not Dynamic)
                return std::addressof(item);
            else if constexpr (std::is_const_v<Item_>)
                return static_cast<Element const*>(item.get());
            else
                return item.get();
        }

        std::vector<Item> items;
    };

} // namespace detail

// Storage for the LinearMap aliases: parallel vectors of elements and ids,
// linear id lookup, swap-and-pop erasure.
ARC_MODULE_EXPORT
template<class Element, class ID, bool Dynamic>
struct VectorStorage : detail::VectorStorageBase<Element, ID, Dynamic>
{
private:
    using Base = detail::VectorStorageBase<Element, ID, Dynamic>;

public:
    using InsertId = ID;

    using Base::contains;

    constexpr void reserve(std::size_t capacity)
    {
        Base::reserve(capacity);
        ids.reserve(capacity);
    }

    [[nodiscard]] constexpr bool contains(ID const& id) const
    {
        return std::ranges::contains(ids, id);
    }

    [[nodiscard]] constexpr auto* findById(this auto& self, ID const& id)
    {
        auto const it = std::find(self.ids.begin(), self.ids.end(), id);
        return it != self.ids.end()
            ? Base::itemToPtr(self.items[std::distance(self.ids.begin(), it)])
            : nullptr;
    }

    // Element is constructed as Element(id, args...)
    constexpr Element* emplace(ID const& id, auto&&... args)
    {
        Element* result = nullptr;
        if constexpr (Dynamic)
            result = this->items.emplace_back(std::make_shared<Element>(id, ARC_FWD(args)...)).get();
        else
            result = std::addressof(this->items.emplace_back(id, ARC_FWD(args)...));
        ids.push_back(id);
        return result;
    }

    constexpr bool erase(ID const& id)
    {
        auto const it = std::find(ids.begin(), ids.end(), id);
        if (it == ids.end())
            return false;
        auto const index = std::distance(ids.begin(), it);
        if (auto const backIdIt = std::prev(ids.end()); it != backIdIt)
        {
            std::iter_swap(it, backIdIt);
            std::iter_swap(std::next(this->items.begin(), index), std::prev(this->items.end()));
        }
        ids.pop_back();
        this->items.pop_back();
        return true;
    }

    // pred: (Element const&) -> bool
    constexpr void eraseIf(auto&& pred)
    {
        auto itEl = this->items.begin();
        auto itId = ids.begin();
        for (; itEl != this->items.end();)
        {
            if (pred(*Base::itemToPtr(*itEl)))
            {
                if (auto const backElIt = std::prev(this->items.end()); backElIt != itEl)
                {
                    auto const backIdIt = std::prev(ids.end());
                    std::iter_swap(itEl, backElIt);
                    std::iter_swap(itId, backIdIt);
                }
                this->items.pop_back();
                ids.pop_back();
            }
            else
            {
                ++itEl;
                ++itId;
            }
        }
    }

private:
    std::vector<ID> ids;
};

// Storage where the unsigned integral ID is the element's index in the slot vector.
// Ids cannot be chosen by the caller: insertion takes arc::autoId and the storage
// assigns the most recently freed index, or the next unused one.
ARC_MODULE_EXPORT
template<class Element, class ID, bool Dynamic>
struct IndexStorage : detail::VectorStorageBase<Element, ID, Dynamic>
{
    ARC_STATIC_ASSERT(std::unsigned_integral<ID>)("IndexStorage requires an unsigned integral ID",
        (id, arc::typeName<ID>)(element, arc::typeName<Element>));

private:
    using Base = detail::VectorStorageBase<Element, ID, Dynamic>;

public:
    using InsertId = AutoId;

    using Base::contains;

    // Number of occupied slots
    [[nodiscard]] constexpr std::size_t size() const noexcept
    {
        if constexpr (Dynamic)
            return this->items.size() - freeIds.size();
        else
            return this->items.size();
    }

    constexpr void ensureSpareCapacity() const
    {
        Base::ensureSpareCapacity();
        if constexpr (Dynamic)
        {
            // Freed ids are reused before extending, so a full id space still has spare capacity
            if (not freeIds.empty())
                return;
        }
        if (this->items.size() > std::numeric_limits<ID>::max())
            throw std::length_error("IndexStorage capacity exceeded (ID type too small)");
    }

    [[nodiscard]] constexpr bool contains(ID const& id) const
    {
        if constexpr (Dynamic)
            return findById(id) != nullptr;
        else
            return id < this->items.size();
    }

    [[nodiscard]] constexpr auto* findById(this auto& self, ID const& id)
    {
        // A freed dynamic slot holds nullptr, so itemToPtr already yields the correct result
        return id < self.items.size()
            ? Base::itemToPtr(self.items[id])
            : nullptr;
    }

    // Element is constructed as Element(assignedId, args...)
    constexpr Element* emplace(AutoId, auto&&... args)
    {
        if constexpr (Dynamic)
        {
            if (not freeIds.empty())
            {
                auto const id = freeIds.back();
                auto& slot = this->items[id];
                slot = std::make_shared<Element>(id, ARC_FWD(args)...);
                freeIds.pop_back();
                return slot.get();
            }
        }

        if (this->items.size() > std::numeric_limits<ID>::max())
            return nullptr;

        auto const id = static_cast<ID>(this->items.size());
        if constexpr (Dynamic)
            return this->items.emplace_back(std::make_shared<Element>(id, ARC_FWD(args)...)).get();
        else
            return std::addressof(this->items.emplace_back(id, ARC_FWD(args)...));
    }

    constexpr bool erase(ID const& id) requires Dynamic
    {
        if (not contains(id))
            return false;
        this->items[id].reset();
        freeIds.push_back(id);
        return true;
    }

    // pred: (Element const&) -> bool
    constexpr void eraseIf(auto&& pred) requires Dynamic
    {
        // Indexed by std::size_t: an ID-typed counter would wrap and never terminate
        // when the id space is exactly full (e.g. 256 slots with a uint8_t ID)
        for (std::size_t id = 0; id < this->items.size(); ++id)
        {
            if (this->items[id] and pred(*Base::itemToPtr(this->items[id])))
            {
                this->items[id].reset();
                freeIds.push_back(static_cast<ID>(id));
            }
        }
    }

    // Range of Element pointers (or shared_ptrs) supporting `el->member` access,
    // skipping freed slots
    [[nodiscard]] constexpr decltype(auto) view(this auto& self)
    {
        if constexpr (Dynamic)
            return self.items | std::views::filter([](auto const& slot) { return slot != nullptr; });
        else
            return self.items | std::views::transform([](auto& el) { return std::addressof(el); });
    }

private:
    ARC_NO_UNIQUE_ADDRESS std::conditional_t<Dynamic, std::vector<ID>, detail::Void> freeIds;
};

// Dynamic layout maps ID -> std::shared_ptr<Element>: elements never move, so any
// map is safe, including one that relocates its entries.
// Static layout keeps elements inline in a vector that never relocates, indexed by
// MapTmpl<ID, std::size_t>.
ARC_MODULE_EXPORT
template<class Element, class ID, bool Dynamic, template<class Key, class Value, class...> class MapTmpl = std::unordered_map>
struct MapStorage;

namespace detail {

    template<class Map>
    constexpr void reserveMap(Map& map, std::size_t capacity)
    {
        if constexpr (requires { map.reserve(capacity); })
            map.reserve(capacity);
    }

    // Projection to an entry's mapped value, for both pair and proxy-pair references
    struct MappedOf
    {
        template<class Entry>
        constexpr auto& operator()(Entry&& entry) const noexcept { return entry.second; }
    };

    template<class Storage, class ID>
    constexpr void assertNewId(Storage const& storage, ID const& id)
    {
        if constexpr (DefaultAssertHandler::enabled)
            DefaultAssertHandler{}(nullptr, not storage.contains(id), "ARC precondition failure: MapStorage::emplace with an id already present");
    }

} // namespace detail

template<class Element, class ID, template<class Key, class Value, class...> class MapTmpl>
struct MapStorage<Element, ID, false, MapTmpl> : detail::VectorStorageBase<Element, ID, false>
{
private:
    using Base = detail::VectorStorageBase<Element, ID, false>;

public:
    using InsertId = ID;

    using Base::contains;

    constexpr void reserve(std::size_t capacity)
    {
        Base::reserve(capacity);
        detail::reserveMap(index, capacity);
    }

    [[nodiscard]] constexpr bool contains(ID const& id) const
    {
        return index.find(id) != index.end();
    }

    [[nodiscard]] constexpr auto* findById(this auto& self, ID const& id)
    {
        auto const it = self.index.find(id);
        return it != self.index.end()
            ? std::addressof(self.items[it->second])
            : nullptr;
    }

    // Element is constructed as Element(id, args...)
    constexpr Element* emplace(ID const& id, auto&&... args)
    {
        detail::assertNewId(*this, id);
        auto& element = this->items.emplace_back(id, ARC_FWD(args)...);
        try
        {
            index.emplace(id, this->items.size() - 1);
        }
        catch (...)
        {
            this->items.pop_back();
            throw;
        }
        return std::addressof(element);
    }

    // Range of Element pointers in the map's order
    [[nodiscard]] constexpr auto view(this auto& self)
    {
        return self.index
            | std::views::transform([items = &self.items](auto&& entry) { return std::addressof((*items)[detail::MappedOf{}(entry)]); });
    }

private:
    MapTmpl<ID, std::size_t> index;
};

template<class Element, class ID, template<class Key, class Value, class...> class MapTmpl>
struct MapStorage<Element, ID, true, MapTmpl> : detail::DynamicHandles<Element, ID>
{
private:
    using Base = detail::DynamicHandles<Element, ID>;

public:
    using InsertId = ID;

    using Base::contains;

    constexpr void reserve(std::size_t capacity)
    {
        detail::reserveMap(map, capacity);
    }

    [[nodiscard]] constexpr std::size_t size() const noexcept { return map.size(); }
    // Elements are individually heap-allocated, so capacity never constrains insertion
    [[nodiscard]] constexpr std::size_t capacity() const noexcept { return std::numeric_limits<std::size_t>::max(); }
    constexpr void ensureSpareCapacity() const noexcept {}

    [[nodiscard]] constexpr bool contains(ID const& id) const
    {
        return map.find(id) != map.end();
    }

    template<class Self>
    [[nodiscard]] constexpr detail::ConstLike<Self, Element>* findById(this Self& self, ID const& id)
    {
        auto const it = self.map.find(id);
        return it != self.map.end() ? it->second.get() : nullptr;
    }

    // Element is constructed as Element(id, args...)
    constexpr Element* emplace(ID const& id, auto&&... args)
    {
        detail::assertNewId(*this, id);
        return map.emplace(id, std::make_shared<Element>(id, ARC_FWD(args)...)).first->second.get();
    }

    // Erased elements are destroyed only once the map is consistent again
    constexpr bool erase(ID const& id)
    {
        auto const it = map.find(id);
        if (it == map.end())
            return false;
        auto const element = std::move(it->second);
        map.erase(it);
        return true;
    }

    // pred: (Element const&) -> bool
    constexpr void eraseIf(auto&& pred)
    {
        for (auto it = map.begin(); it != map.end();)
        {
            auto& slot = detail::MappedOf{}(*it);
            if (pred(*slot))
            {
                auto const element = std::move(slot);
                it = map.erase(it);
            }
            else
                ++it;
        }
    }

    // Range of Element shared_ptrs in the map's order, supporting `el->member` access
    [[nodiscard]] constexpr auto view(this auto& self)
    {
        return self.map | std::views::transform(detail::MappedOf{});
    }

private:
    MapTmpl<ID, std::shared_ptr<Element>> map;
};

// Collection policy: bundles element lifetime (dynamic vs static) with the
// storage layout used for elements and ids.
ARC_MODULE_EXPORT
template<bool Dynamic, template<class Element, class ID, bool Dynamic_> class StorageTemplate>
struct CollectionPolicy
{
    static constexpr bool dynamic = Dynamic;

    template<class Element, class ID>
    using Storage = StorageTemplate<Element, ID, Dynamic>;
};

using StaticLinearMapPolicy = CollectionPolicy<false, VectorStorage>;
using DynamicLinearMapPolicy = CollectionPolicy<true, VectorStorage>;

namespace detail {

    template<template<class Key, class Value, class...> class MapTmpl>
    struct MapStorageOf
    {
        template<class Element, class ID, bool Dynamic>
        using Tmpl = MapStorage<Element, ID, Dynamic, MapTmpl>;
    };

} // namespace detail

template<template<class Key, class Value, class...> class MapTmpl>
using StaticMapPolicy = CollectionPolicy<false, detail::MapStorageOf<MapTmpl>::template Tmpl>;
template<template<class Key, class Value, class...> class MapTmpl>
using DynamicMapPolicy = CollectionPolicy<true, detail::MapStorageOf<MapTmpl>::template Tmpl>;

// Element ids are indices assigned by the storage; insertion takes arc::autoId
using StaticIndexPolicy = CollectionPolicy<false, IndexStorage>;
using DynamicIndexPolicy = CollectionPolicy<true, IndexStorage>;

} // namespace arc

#endif // INCLUDE_ARC_NODES_COLLECTION_STORAGE_HPP
