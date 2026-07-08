#ifndef INCLUDE_ARC_NODES_COLLECTION_STORAGE_HPP
#define INCLUDE_ARC_NODES_COLLECTION_STORAGE_HPP

#include "arc/empty_types.hpp"
#include "arc/macros.hpp"

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
        constexpr explicit WeakDynamicHandle(std::weak_ptr<Element> element) : ptr(element.lock()) {}
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

    // Common base for vector-backed collection storages. Owns the item vector and
    // all handle construction/inspection; the derived storage provides the id scheme:
    // findById, emplace, erase, eraseIf, and any id bookkeeping.
    // Dynamic layout heap-allocates elements (shared_ptr) so handles stay stable;
    // static layout stores elements inline and never relocates them.
    // Element is unique per collection node, so handles remain distinct types.
    template<class Element, class ID, bool Dynamic>
    struct VectorStorageBase
    {
        using WeakHandle = std::conditional_t<Dynamic,
            detail::WeakDynamicHandle<VectorStorageBase, Element>,
            detail::StaticHandle<VectorStorageBase>>;
        using StrongHandle = std::conditional_t<Dynamic,
            detail::DynamicHandle<VectorStorageBase, Element>,
            detail::StaticHandle<VectorStorageBase>>;

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

        [[nodiscard]] constexpr bool contains(this auto const& self, StrongHandle const& handle)
        {
            if constexpr (Dynamic)
                return handle.ptr and self.findById(handle.ptr->id) == handle.ptr.get();
            else
                return handle.index < self.items.size();
        }

        // Range of Element pointers (or shared_ptrs) supporting `el->member` access
        [[nodiscard]] constexpr decltype(auto) view(this auto& self)
        {
            if constexpr (Dynamic)
                return (self.items);
            else
                return self.items | std::views::transform([](auto& el) { return std::addressof(el); });
        }

        [[nodiscard]] constexpr StrongHandle handleFor(Element const* element) const
        {
            if constexpr (Dynamic)
            {
                // const_cast is safe since the handle is opaque and consumers enforce const correctness at point of use
                return element ? StrongHandle(const_cast<Element*>(element)->shared_from_this()) : StrongHandle();
            }
            else
            {
                auto const index = element - std::to_address(items.begin());
                return StrongHandle(static_cast<std::size_t>(index));
            }
        }

        [[nodiscard]] constexpr WeakHandle weakHandleFor(this auto const& self, ID const& id) requires Dynamic
        {
            return WeakHandle(lockElement(self.findById(id)));
        }
        [[nodiscard]] constexpr StrongHandle strongHandleFor(this auto const& self, ID const& id) requires Dynamic
        {
            return StrongHandle(lockElement(self.findById(id)));
        }

        [[nodiscard]] static constexpr ID idOf(StrongHandle const& handle) requires Dynamic
        {
            return handle.ptr->id;
        }

        [[nodiscard]] constexpr auto* elementOf(this auto& self, StrongHandle const& handle)
        {
            if constexpr (Dynamic)
                return handle.ptr.get();
            else
                return std::addressof(self.items[handle.index]);
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

    private:
        [[nodiscard]] static constexpr std::shared_ptr<Element> lockElement(Element const* element) requires Dynamic
        {
            // const_cast is safe since the handle is opaque and consumers enforce const correctness at point of use
            return element ? const_cast<Element*>(element)->shared_from_this() : nullptr;
        }
    };

} // namespace detail

// Default storage for arc::Collection: parallel vectors of elements and ids,
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
        ids.push_back(id);
        if constexpr (Dynamic)
            return this->items.emplace_back(std::make_shared<Element>(id, ARC_FWD(args)...)).get();
        else
            return std::addressof(this->items.emplace_back(id, ARC_FWD(args)...));
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
            if (pred(*Base::itemToPtr(std::as_const(*itEl))))
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
    static_assert(std::unsigned_integral<ID>, "IndexStorage requires an unsigned integral ID");

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
                freeIds.pop_back();
                auto& slot = this->items[id];
                slot = std::make_shared<Element>(id, ARC_FWD(args)...);
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
            if (this->items[id] and pred(std::as_const(*this->items[id])))
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
    [[no_unique_address]] std::conditional_t<Dynamic, std::vector<ID>, detail::Void> freeIds;
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

using StaticMapPolicy = CollectionPolicy<false, VectorStorage>;
using DynamicMapPolicy = CollectionPolicy<true, VectorStorage>;

// Element ids are indices assigned by the storage; insertion takes arc::autoId
using StaticIndexPolicy = CollectionPolicy<false, IndexStorage>;
using DynamicIndexPolicy = CollectionPolicy<true, IndexStorage>;

} // namespace arc

#endif // INCLUDE_ARC_NODES_COLLECTION_STORAGE_HPP
