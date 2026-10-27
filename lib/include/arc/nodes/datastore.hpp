#ifndef INCLUDE_ARC_DATA_STORE_HPP
#define INCLUDE_ARC_DATA_STORE_HPP


#include "arc/detail/bitset.hpp"
#include "arc/detail/concepts.hpp"
#include "arc/detail/storage.hpp"
#include "arc/empty_types.hpp"
#include "arc/macros.hpp"
#include "arc/static_assert.hpp"
#include "arc/nodes/collection_fwd.hpp"
#include "arc/nodes/repeater.hpp"

#include "arc/traits/datastore.hpp"
#include "arc/detail/with_index.hpp"

#if !ARC_IMPORT_STD
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <limits>
#include <memory>
#include <optional>
#include <tuple>
#include <type_traits>
#include <unordered_map>
#include <utility>
#endif

#if ARC_ASAN_ENABLED
extern "C" void __asan_poison_memory_region(void const volatile* address, std::size_t size);
extern "C" void __asan_unpoison_memory_region(void const volatile* address, std::size_t size);
#endif

namespace arc {

ARC_MODULE_EXPORT
using NoPrivateData = arc::detail::Void;

ARC_MODULE_EXPORT
struct NoEvent final {} inline constexpr noEvent{};

namespace detail {

    template<class T>
    concept IsNotEvent = IsInstanceOf<T, NoEvent> or std::is_void_v<T>;

    template<class F>
    struct UseDataSwallowId
    {
        F&& f;
        template<class Id, class SharedData, class PrivateData>
        constexpr decltype(auto) operator()(Id const&, SharedData* sharedData, PrivateData* privateData)
        {
            return ARC_FWD(f)(sharedData, privateData);
        }
    };

    template<class F>
    UseDataSwallowId(F&&) -> UseDataSwallowId<F>;

    inline void poisonMemory([[maybe_unused]] void const volatile* address, [[maybe_unused]] std::size_t size) noexcept
    {
        #if ARC_ASAN_ENABLED
        __asan_poison_memory_region(address, size);
        #endif
    }

    inline void unpoisonMemory([[maybe_unused]] void const volatile* address, [[maybe_unused]] std::size_t size) noexcept
    {
        #if ARC_ASAN_ENABLED
        __asan_unpoison_memory_region(address, size);
        #endif
    }

    template<class T>
    constexpr bool isEvent(std::optional<T> const& event)
    {
        return event.has_value();
    }
    template<class T>
    constexpr std::true_type isEvent(T const&)
    {
        ARC_STATIC_ASSERT_INLINE(not IsNotEvent<T>)("Type returned from useData callback is not an event",
            (type, arc::typeName<T>));
        return {};
    }

    template<class T>
    constexpr T const& getEvent(std::optional<T> const& event)
    {
        return *event;
    }
    template<class T>
    constexpr T const& getEvent(T const& event)
    {
        ARC_STATIC_ASSERT_INLINE(not IsNotEvent<T>)("Type returned from useData callback is not an event",
            (type, arc::typeName<T>));
        return event;
    }

    consteval std::size_t alignUp(std::size_t size, std::size_t alignment)
    {
        return (size + alignment - 1) / alignment * alignment;
    }

} // namespace detail

// Entries live in slots that never move, and the map only maps each Id to a slot index, so any map is safe, including
// one that relocates its own entries. A slot is reused only once no ref pins it, and the slot's generation changes
// whenever its id is removed, which is what makes a Handle go stale.
ARC_MODULE_EXPORT
template<class SharedData, template<class Key, class Value, class...> class MapTmpl = std::unordered_map>
struct DataStoreNode
{
    using Id = SharedData::Id;
    using DataStoreTrait = trait::DataStore<SharedData>;
    using DataListenerTrait = trait::DataListener<SharedData>;

    static constexpr std::size_t noListener = std::numeric_limits<std::size_t>::max();
    static constexpr std::uint8_t noInit = std::numeric_limits<std::uint8_t>::max();
    static constexpr std::uint32_t noSlot = std::numeric_limits<std::uint32_t>::max();

    template<class Context>
    struct Node : arc::NodeImpl<DataStoreTrait>
    {
        using Depends = arc::Depends<DataListenerTrait>;

    private:
        using Handle = DataHandle<SharedData>;
        class Ref;
        template<class PrivateDataPtr>
        struct PrivateRefOf;
        template<class PrivateDataPtr>
        struct AddedOf;

        // The context where the store's DataListener calls arrive. It is a context rather than a node, so that no
        // listener's class is completed while the store's class is still being defined.
        using ListenerContext = detail::LinkTarget<ContextOf<Node>, DataListenerTrait>::Context;

        static consteval std::size_t dataListenerCount()
        {
            constexpr std::size_t repeated = detail::repeaterCount<typename ListenerContext::NodeHandle>;
            if constexpr (repeated == 0)
            {
                return 1;
            }
            else
            {
                ARC_STATIC_ASSERT_INLINE(repeated > 1)("DataStore behind a repeater requires at least two DataListeners",
                    (shared_data, arc::typeName<SharedData>)(listener_count, repeated));
                return repeated;
            }
        }

        static constexpr std::size_t DataListenerCount = dataListenerCount();
        ARC_STATIC_ASSERT(DataListenerCount <= 64)("A DataStore has at most 64 DataListeners",
            (shared_data, arc::typeName<SharedData>)
            (listener_count, DataListenerCount));

        using IndexSequence = std::make_index_sequence<DataListenerCount>;
        using Bitset = detail::Bitset<DataListenerCount>;

        template<std::size_t I>
        static auto listenerTypesAt()
        {
            if constexpr (DataListenerCount == 1)
                return std::type_identity<arc::ResolveTypes<Node, DataListenerTrait>>{};
            else
                return std::type_identity<arc::ResolveTypes<Node, DataListenerTrait, key::RepeaterIndex<I>>>{};
        }

        template<std::size_t I>
        requires (I < DataListenerCount)
        using PrivateDataAt = decltype(listenerTypesAt<I>())::type::PrivateData;

        template<std::size_t I>
        requires (I == noListener)
        static consteval void privateDataOf();
        template<std::size_t I>
        requires (I != noListener)
        static consteval auto privateDataOf()
        {
            return ARC_DECLVAL(PrivateDataAt<I>);
        }
        template<std::size_t I>
        using PrivateDataOf = decltype(privateDataOf<I>());

        template<std::size_t I, bool Const>
        using PrivateDataPtrAt = std::conditional_t<Const, PrivateDataOf<I> const, PrivateDataOf<I>>*;

        template<std::size_t I, class PrivateData_ = PrivateDataOf<I>>
        struct TypesFor
        {
            using PrivateData = PrivateData_;
            using Handle = Node::Handle;
            using Ref = Node::Ref;
            using PrivateRef = PrivateRefOf<PrivateData*>;
            using ConstPrivateRef = PrivateRefOf<PrivateData const*>;
            using Added = AddedOf<PrivateData*>;
        };

    public:
        using Types = TypesFor<noListener>;

    private:
        template<std::size_t I>
        static auto listenerContextAt()
        {
            if constexpr (DataListenerCount == 1)
                return std::type_identity<ListenerContext>{};
            else
                return std::type_identity<typename detail::LinkTarget<ListenerContext, RepeaterTrait<I>>::Context>{};
        }
        template<std::size_t I>
        using ListenerContextAt = decltype(listenerContextAt<I>())::type;

        // Whether a collection receives the DataListener calls arriving at context C, directly or through clusters
        template<class C>
        static consteval bool collectionReceives()
        {
            using Receiver = ContextToNode<C>;
            if constexpr (detail::isCollection<typename C::NodeHandle>)
                return true;
            else if constexpr (detail::HasExplicitLink<Receiver, DataListenerTrait>)
                return collectionReceives<detail::ResolveLinkContext<Receiver, DataListenerTrait>>();
            else
                return false;
        }

        template<std::size_t... Is>
        static consteval bool noListenerIsCollection(std::index_sequence<Is...>)
        {
            return (... and not collectionReceives<ListenerContextAt<Is>>());
        }

        // Found from the store's links and the Source's Traits alone, never the Source's other members, so that a node
        // can resolve the store's types while its own class is still being defined
        template<class Source, std::size_t... Is>
        static consteval std::size_t listenerIndexOf(std::index_sequence<Is...>)
        {
            if constexpr (not detail::TraitsHasTrait<typename Source::Traits, DataListenerTrait>)
            {
                return noListener;
            }
            else
            {
                using detail::LinkReach;
                constexpr LinkReach reach[] = {detail::linkReach<ContextOf<Source>, ListenerContextAt<Is>, DataListenerTrait>()...};
                ARC_STATIC_ASSERT_INLINE(((reach[Is] != LinkReach::ThroughElement) and ...))(
                    "A DataListener inside a collection is not supported: its elements would share one PrivateData per entry",
                    (shared_data, arc::typeName<SharedData>)
                    ARC_SA_NODE_CONTEXT(element, ContextOf<Source>));
                ARC_STATIC_ASSERT_INLINE(((reach[Is] == LinkReach::ThroughElement) or ...) or (std::size_t{reach[Is] == LinkReach::Yes} + ...) == 1)(
                    "A DataListener of the DataStore it reaches must be reached by exactly one of the store's DataListener links",
                    (shared_data, arc::typeName<SharedData>)
                    (listeners, arc::typeName<void(ListenerContextAt<Is>...)>)
                    ARC_SA_NODE_CONTEXT(source, ContextOf<Source>));
                std::size_t index = noListener;
                ((index = reach[Is] == LinkReach::Yes ? Is : index), ...);
                return index;
            }
        }

        template<class Source>
        static constexpr std::size_t ListenerIndexOf = listenerIndexOf<Source>(IndexSequence{});

        // Read from the listener's own DataListener types rather than through the store's link to the listener, because
        // that link may be what is completing the listener's class
        template<class Source>
        requires (ListenerIndexOf<Source> == noListener)
        static auto sourcePrivateData() -> std::type_identity<void>;
        template<class Source>
        requires (ListenerIndexOf<Source> != noListener)
        static auto sourcePrivateData() -> std::type_identity<typename Source::Traits::template ResolveTypes<DataListenerTrait, Source>::PrivateData>;

    public:
        // Declared before anything here completes a listener's class: a listener resolving the store's types while its
        // class is being defined would otherwise find arc::Node's FinaliseTypes
        template<class Source, class Self, class Types, class... Keys>
        using FinaliseTypes = TypesFor<ListenerIndexOf<Source>, typename decltype(sourcePrivateData<Source>())::type>;

    private:
        ARC_STATIC_ASSERT(noListenerIsCollection(IndexSequence{}))("A collection as a DataListener is not supported: its elements would share one PrivateData per entry",
            (shared_data, arc::typeName<SharedData>));

        struct Entry
        {
            constexpr explicit Entry(Id const& id, auto&&... sharedDataArgs)
                : id(id)
                , sharedData{ARC_FWD(sharedDataArgs)...}
            {}

            Id id;
            ARC_NO_UNIQUE_ADDRESS SharedData sharedData;
        };

        template<std::size_t... Is>
        static auto privateDataTuple(std::index_sequence<Is...>) -> std::tuple<detail::Storage<PrivateDataAt<Is>>...>;
        using PrivateDataTuple = decltype(privateDataTuple(IndexSequence{}));

        // Everything a freed slot poisons
        struct Body
        {
            detail::Storage<Entry> entry;
            PrivateDataTuple privateData;
        };

        template<bool BitsetAfterCounters, class = void>
        struct SlotLayout;

        // The metadata follows the Body in ascending alignment, so that the only padding is at the end of the slot.
        // The Body comes first so that it starts on an ASan granule.
        template<class Unused>
        struct SlotLayout<false, Unused>
        {
            constexpr explicit SlotLayout(std::uint32_t index) : index(index) {}

            Body body;
            bool linked = false;
            std::uint8_t initListener = noInit; // whose init the outer add is running
            Bitset engaged;
            Bitset alive;
            std::uint32_t pins = 0;
            std::uint32_t generation = 0;
            std::uint32_t const index;
            union
            {
                std::uint64_t birth = 0; // when linked
                std::uint32_t nextFree; // when free
            };
            std::size_t eventId = 0;
        };

        template<class Unused>
        struct SlotLayout<true, Unused>
        {
            constexpr explicit SlotLayout(std::uint32_t index) : index(index) {}

            Body body;
            bool linked = false;
            std::uint8_t initListener = noInit; // whose init the outer add is running
            std::uint32_t pins = 0;
            std::uint32_t generation = 0;
            std::uint32_t const index;
            Bitset engaged;
            Bitset alive;
            union
            {
                std::uint64_t birth = 0; // when linked
                std::uint32_t nextFree; // when free
            };
            std::size_t eventId = 0;
        };

        // A slot is free, linked (the map holds its id), or unlinked but still pinned after a release. The entry is
        // alive unless the slot is free; listener I's PrivateData is alive iff alive.test(I); engaged is a subset of alive.
        struct Slot : SlotLayout<(alignof(Bitset) > alignof(std::uint32_t))>
        {
            using SlotLayout<(alignof(Bitset) > alignof(std::uint32_t))>::SlotLayout;
            Slot(Slot const&) = delete;
            Slot& operator=(Slot const&) = delete;

            Entry& entry() { return *this->body.entry.value(); }
            Entry const& entry() const { return *this->body.entry.value(); }

            template<std::size_t I>
            PrivateDataAt<I>* privateData() { return std::get<I>(this->body.privateData).value(); }
            template<std::size_t I>
            PrivateDataAt<I> const* privateData() const { return std::get<I>(this->body.privateData).value(); }
        };

        static constexpr std::size_t SlotMetadataSize = sizeof(bool) + sizeof(std::uint8_t) + 2 * sizeof(Bitset)
            + 3 * sizeof(std::uint32_t) + sizeof(std::uint64_t) + sizeof(std::size_t);
        static_assert(alignof(Slot) == (alignof(Body) > alignof(std::uint64_t) ? alignof(Body) : alignof(std::uint64_t)));
        static_assert(sizeof(Slot) == detail::alignUp(sizeof(Body) + SlotMetadataSize, alignof(Slot)));

        struct Pin
        {
            // Pinning is bookkeeping, done by const lookups too
            Pin(Node const& store, Slot* slot) noexcept
                : store(const_cast<Node*>(std::addressof(store)))
                , slot(slot)
            {
                if (slot)
                    ++slot->pins;
            }
            Pin(Pin const&) = delete;
            Pin& operator=(Pin const&) = delete;
            ~Pin()
            {
                if (slot and --slot->pins == 0)
                    store->reap(*slot);
            }

            Node* store;
            Slot* slot;
        };

        static constexpr std::size_t ListenerIndex = noListener;

        class Ref
        {
        public:
            Ref(Ref const&) = delete;
            Ref& operator=(Ref const&) = delete;
            static void* operator new(std::size_t) = delete;
            static void* operator new[](std::size_t) = delete;

            explicit operator bool() const noexcept { return pin.slot != nullptr; }

            SharedData const& operator*() const& ARC_LIFETIMEBOUND { return slot().entry().sharedData; }
            SharedData const& operator*() const&& = delete;
            SharedData const* operator->() const ARC_LIFETIMEBOUND { return std::addressof(slot().entry().sharedData); }

            Id const& id() const ARC_LIFETIMEBOUND { return slot().entry().id; }

            // Empty once released: a released slot's generation is the one its next entry will carry
            Handle handle() const noexcept
            {
                return pin.slot and pin.slot->linked ? pin.store->handleOf(*pin.slot) : Handle(Handle::noSlot, 0);
            }

            // Another Ref pinning the same entry: copying is explicit, as every copy extends the entry's lifetime
            Ref clone() const noexcept { return Ref(*pin.store, pin.slot); }

        private:
            friend Node;
            Ref(Node const& store, Slot* slot) noexcept : pin(store, slot) {}

            Slot& slot() const
            {
                check(pin.slot != nullptr, "ARC precondition failure: dereferencing an empty arc::DataStore Ref");
                return *pin.slot;
            }

            Pin pin;
        };

        // Reads like the derived struct's ref. Holds no data, so the derived struct binds as [ref, privateData...].
        struct ForwardsToRef
        {
            static void* operator new(std::size_t) = delete;
            static void* operator new[](std::size_t) = delete;

            template<class Self>
            explicit operator bool(this Self const& self) noexcept { return static_cast<bool>(self.ref); }

            template<class Self>
            SharedData const& operator*(this Self const& self ARC_LIFETIMEBOUND) { return *self.ref; }
            template<class Self>
            SharedData const& operator*(this Self const&& self) = delete;
            template<class Self>
            SharedData const* operator->(this Self const& self ARC_LIFETIMEBOUND) { return self.ref.operator->(); }

            template<class Self>
            Id const& id(this Self const& self ARC_LIFETIMEBOUND) { return self.ref.id(); }
            template<class Self>
            Handle handle(this Self const& self) noexcept { return self.ref.handle(); }
            template<class Self>
            Ref clone(this Self const& self) noexcept { return self.ref.clone(); }
        };

        template<class PrivateDataPtr>
        struct PrivateRefOf : ForwardsToRef
        {
            Ref ref;
            PrivateDataPtr privateData;
        };

        template<class PrivateDataPtr>
        struct AddedOf : ForwardsToRef
        {
            Ref ref;
            PrivateDataPtr privateData;
            bool inserted;
        };

        template<class Self>
        using PrivateArg = PrivateDataPtrAt<Self::ListenerIndex, std::is_const_v<Self>>;

        template<class Self>
        using RefFor = PrivateRefOf<PrivateArg<Self>>;

        template<class Self>
        using CallerTypes = TypesFor<Self::ListenerIndex>;

        std::deque<Slot> slots;
        MapTmpl<Id, std::uint32_t> index;
        std::uint32_t freeHead = noSlot;
        std::uint64_t births = 0;

        template<std::size_t ListenerIndex>
        struct FromDataListener;

        template<std::size_t I>
        constexpr auto getDataListener(this auto& self, std::integral_constant<std::size_t, I> = {})
        {
            if constexpr (DataListenerCount == 1)
                return self.getNode(DataListenerTrait{});
            else
                return self.getNode(DataListenerTrait{}, key::repeaterIndex<I>);
        }

        static constexpr void check(bool condition, char const* message)
        {
            if constexpr (Context::Info::ContractAssert.enabled)
                Context::Info::ContractAssert(nullptr, condition, message);
        }

        // Released PrivateData stays alive while pinned, but is unreachable
        template<class Self>
        static PrivateArg<Self> privateDataIn(Slot* slot) noexcept
        {
            constexpr std::size_t I = Self::ListenerIndex;
            if constexpr (I != noListener)
                if (slot and slot->engaged.test(I))
                    return slot->template privateData<I>();
            return nullptr;
        }

        template<class Self>
        static RefFor<Self> makeRef(Node const& store, Slot* slot) noexcept
        {
            return {{}, Ref(store, slot), privateDataIn<Self>(slot)};
        }

        template<class Self>
        static AddedOf<PrivateArg<Self>> makeAdded(Node const& store, Slot* slot, bool inserted) noexcept
        {
            return {{}, Ref(store, slot), privateDataIn<Self>(slot), inserted};
        }

        Slot* find(Id const& id) const
        {
            auto const it = index.find(id);
            return it != index.end() ? const_cast<Slot*>(std::addressof(slots[it->second])) : nullptr;
        }

        static constexpr Handle handleOf(Slot const& slot) noexcept
        {
            return Handle(slot.index, slot.generation);
        }

        // The slot a ref reads, when it was taken from this store instance
        Slot* slotOf(Ref const& ref) const
        {
            check(ref.pin.slot == nullptr or ref.pin.store == this, "ARC precondition failure: arc::DataStore Ref used with a store it did not come from");
            return ref.pin.slot;
        }
        template<class PrivateDataPtr>
        Slot* slotOf(PrivateRefOf<PrivateDataPtr> const& ref) const
        {
            return slotOf(ref.ref);
        }

        template<class Self>
        static RefFor<Self> refIfLinked(Node const& store, Slot& slot, std::uint32_t generation) noexcept
        {
            return makeRef<Self>(store, slot.linked and slot.generation == generation ? std::addressof(slot) : nullptr);
        }

        template<std::size_t... Is>
        static consteval bool privateDataCopyable(std::index_sequence<Is...>)
        {
            return (std::copy_constructible<PrivateDataAt<Is>> and ...);
        }

        // Linked only once its entry and PrivateData are constructed, so that destroySlots never meets half a copy
        void copySlot(Slot const& from, Slot& to)
        {
            to.generation = from.generation;
            if (not from.linked)
            {
                putFreeSlot(to);
                return;
            }
            std::construct_at(to.body.entry.value(), from.entry());
            try
            {
                forEachIndex<DataListenerCount>(
                    [&](auto i)
                    {
                        if (not from.engaged.test(i))
                            return;
                        std::construct_at(to.template privateData<i>(), *from.template privateData<i>());
                        to.alive.set(i);
                        to.engaged.set(i);
                    });
            }
            catch (...)
            {
                forEachIndex<DataListenerCount>(
                    [&to](auto i)
                    {
                        if (to.alive.test(i))
                            std::destroy_at(to.template privateData<i>());
                    });
                std::destroy_at(std::addressof(to.entry()));
                throw;
            }
            to.linked = true;
            to.birth = from.birth;
            to.eventId = from.eventId;
        }

        void destroySlots() noexcept
        {
            for (Slot& slot : slots)
            {
                detail::unpoisonMemory(std::addressof(slot.body), sizeof(Body));
                if (not slot.linked)
                    continue;
                forEachIndex<DataListenerCount>(
                    [&slot](auto i)
                    {
                        if (slot.alive.test(i))
                            std::destroy_at(slot.template privateData<i>());
                    });
                std::destroy_at(std::addressof(slot.entry()));
            }
        }

        Slot& takeFreeSlot()
        {
            if (freeHead == noSlot)
                return slots.emplace_back(static_cast<std::uint32_t>(slots.size()));
            Slot& slot = slots[freeHead];
            detail::unpoisonMemory(std::addressof(slot.body), sizeof(Body));
            freeHead = slot.nextFree;
            return slot;
        }

        void putFreeSlot(Slot& slot) noexcept
        {
            detail::poisonMemory(std::addressof(slot.body), sizeof(Body));
            slot.nextFree = freeHead;
            freeHead = slot.index;
        }

        void unlink(Slot& slot)
        {
            index.erase(index.find(slot.entry().id));
            slot.linked = false;
            ++slot.generation;
        }

        // Destroys what was released, once nothing pins it. A pinned slot is never free, so an unlinked one holds an entry.
        void reap(Slot& slot) noexcept
        {
            if (slot.linked and slot.alive == slot.engaged)
                return;
            forEachIndex<DataListenerCount>(
                [&slot](auto i)
                {
                    if (slot.alive.test(i) and not slot.engaged.test(i))
                    {
                        std::destroy_at(slot.template privateData<i>());
                        slot.alive.unset(i);
                    }
                });
            if (not slot.linked)
            {
                std::destroy_at(std::addressof(slot.entry()));
                putFreeSlot(slot);
            }
        }

        template<std::size_t I>
        PrivateDataAt<I>* emplacePrivateData(Slot& slot, auto&& makePrivateData)
        {
            PrivateDataAt<I>* privateData = slot.template privateData<I>();
            if (slot.alive.test(I)) [[unlikely]]
            {
                check(false, "ARC precondition failure: arc::DataStore PrivateData constructed while the listener's previous PrivateData for the id is alive");
                return slot.engaged.test(I) ? privateData : nullptr;
            }
            // Constructed in place from the returned prvalue, so the PrivateData need not be movable
            ::new (static_cast<void*>(privateData)) PrivateDataAt<I>(makePrivateData());
            slot.alive.set(I);
            slot.engaged.set(I);
            return privateData;
        }

        template<std::size_t I>
        constexpr decltype(auto) init(Slot& slot)
        {
            using PrivateData = PrivateDataAt<I>;
            auto const constructor = [this, &slot]<class... Args>(Args&&... args) -> PrivateData*
                requires std::constructible_from<PrivateData, Args...>
            {
                ARC_STATIC_ASSERT_INLINE(detail::alwaysTrue<Args...> and initReturnsVoid<I>())("DataListener init that returns PrivateData must not use the injected constructor",
                    (private_data, arc::typeName<PrivateData>)(listener_index, I));
                if (slot.initListener != I) [[unlikely]]
                {
                    check(false, "ARC precondition failure: arc::DataStore PrivateData constructor called outside the init it was passed to");
                    return nullptr;
                }
                return this->emplacePrivateData<I>(slot, [&] { return PrivateData(ARC_FWD(args)...); });
            };
            Entry& entry = slot.entry();
            return this->getDataListener<I>().init(entry.id, handleOf(slot), entry.sharedData, constructor);
        }

        template<std::size_t I>
        static constexpr bool initReturnsVoid()
        {
            return std::is_void_v<decltype(std::declval<Node>().template init<I>(std::declval<Slot&>()))>;
        }

        // Ends the init phase of add, also when an init throws: an entry that no listener subscribed to is removed
        struct InitScope
        {
            ~InitScope()
            {
                slot.initListener = noInit;
                if (slot.engaged.none() and slot.linked)
                    store.unlink(slot);
            }

            Node& store;
            Slot& slot;
        };

        // Listener Skip, the caller of modify, is not notified
        template<std::size_t Skip>
        bool notifySlot(Slot& slot, auto const& event)
        {
            ++slot.eventId;
            return forAllIndices<DataListenerCount>(
                [this, event = Event{std::addressof(event), &slot.eventId}, &slot](auto i)
                {
                    if constexpr (i != Skip)
                    {
                        if (slot.engaged.test(i))
                        {
                            auto listener = this->getDataListener(i);
                            Entry const& entry = slot.entry();
                            Handle const handle = handleOf(slot);
                            auto& privateData = *slot.template privateData<i>();
                            using EventView = EventItem<SharedData const, PrivateDataAt<i> const>;
                            if (listener.skipEvent(event, EventView{entry.id, handle, entry.sharedData, privateData}))
                                return true;
                            using EventRef = EventItem<SharedData const, PrivateDataAt<i>>;
                            using result_t = decltype(listener.onEvent(event, EventRef{entry.id, handle, entry.sharedData, privateData}));
                            if constexpr (std::is_void_v<result_t>)
                                listener.onEvent(event, EventRef{entry.id, handle, entry.sharedData, privateData});
                            else
                                return listener.onEvent(event, EventRef{entry.id, handle, entry.sharedData, privateData});
                        }
                    }
                    return true;
                });
        }

        // The slot is pinned by the caller
        template<class Self, class UseData>
        static void useData(Self& self, Slot& slot, UseData&& f)
        {
            constexpr std::size_t I = Self::ListenerIndex;
            Entry& entry = slot.entry();
            detail::ConstLike<Self, SharedData>* sharedData = std::addressof(entry.sharedData);
            PrivateArg<Self> privateData = privateDataIn<Self>(std::addressof(slot));

            if constexpr (detail::IsNotEvent<std::invoke_result_t<UseData, Id const&, decltype(sharedData), PrivateArg<Self>>>)
            {
                f(entry.id, static_cast<SharedData const*>(sharedData), privateData);
            }
            else if (decltype(auto) event = f(entry.id, sharedData, privateData); detail::isEvent(event))
            {
                ARC_STATIC_ASSERT_INLINE(not std::is_const_v<Self>)("useData with returned event cannot be called on const DataStore",
                    (shared_data, arc::typeName<SharedData>)(event, arc::typeName<std::remove_cvref_t<decltype(event)>>));
                self.template notifySlot<I>(slot, detail::getEvent(event));
            }
        }

        // Visits only the slots linked when the loop began, by index because slots may be added during the loop, and
        // pins each slot while visiting it
        void forEachSlot(auto&& visit) const
        {
            std::size_t const end = slots.size();
            std::uint64_t const bornBefore = births;
            for (std::size_t i = 0; i != end; ++i)
            {
                Slot& slot = const_cast<Slot&>(slots[i]);
                if (not slot.linked or slot.birth > bornBefore)
                    continue;
                Pin const pin(*this, std::addressof(slot));
                visit(slot);
            }
        }

        template<class Self>
        static constexpr bool IsListenerSelf = Self::ListenerIndex != noListener;

        template<class Self, class... Args>
        auto subscribeSlot(this Self& self, Slot* slot, Args&&... args) -> RefFor<Self>
        {
            constexpr std::size_t I = Self::ListenerIndex;
            if (not slot)
                return makeRef<Self>(self, nullptr);
            if (slot->initListener == I)
            {
                check(false, "ARC precondition failure: arc::DataStore listener subscribed to the id its own init is being offered");
                return makeRef<Self>(self, slot);
            }
            if (not slot->engaged.test(I))
                self.template emplacePrivateData<I>(*slot, [&] { return PrivateDataAt<I>(ARC_FWD(args)...); });
            return makeRef<Self>(self, slot);
        }

        template<class Self>
        bool releaseSlot(this Self& self, Slot* slot)
        {
            constexpr std::size_t I = Self::ListenerIndex;
            if (not slot or not slot->engaged.test(I))
                return false;
            Pin const pin(self, slot);
            slot->engaged.unset(I);
            if (slot->engaged.none() and slot->initListener == noInit)
                self.unlink(*slot);
            return true;
        }

    public:
        Node() = default;

        // No ref may pin other, since refs point into the store they came from. The members are moved only by
        // swaps, which do not throw, so the move cannot fail part way.
        Node(Node&& other)
        {
            if constexpr (Context::Info::ContractAssert.enabled)
                check(std::ranges::none_of(other.slots, [](Slot const& slot) { return slot.pins != 0; }), "ARC precondition failure: arc::DataStore moved while a Ref pins it");
            using std::swap;
            swap(index, other.index);
            slots.swap(other.slots);
            swap(freeHead, other.freeHead);
            swap(births, other.births);
        }

        // The copy keeps the same slots and generations, so handles taken from other resolve in the copy. Nothing in
        // the copy is pinned, since refs belong to the store they came from.
        Node(Node const& other)
            requires (std::copy_constructible<SharedData> and std::copy_constructible<MapTmpl<Id, std::uint32_t>> and privateDataCopyable(IndexSequence{}))
            : index(other.index)
            , births(other.births)
        {
            try
            {
                for (Slot const& from : other.slots)
                    copySlot(from, slots.emplace_back(from.index));
            }
            catch (...)
            {
                destroySlots();
                throw;
            }
        }

        ~Node() { destroySlots(); }

        template<class Source, class Key = ContextOf<Source>::Info::DefaultKey>
        ARC_INLINE constexpr auto finalise(this auto& self, Source& source, Key const& key = {}, auto const&... keys)
        {
            if constexpr (detail::TraitsHasTrait<typename Source::Traits, DataListenerTrait>)
                return Context::Info::finalise(source, detail::downCast<FromDataListener<ListenerIndexOf<Source>>>(self), key, keys...);
            else
                return Context::Info::finalise(source, self, key, keys...);
        }

        template<class Self, class... SharedArgs>
        requires std::constructible_from<SharedData, SharedArgs...>
        auto add(this Self& self, Id const& id, SharedArgs&&... sharedArgs) -> AddedOf<PrivateArg<Self>>
        {
            Node& store = self;
            Slot* slot = store.find(id);
            if (slot and (slot->initListener != noInit or slot->alive.all()))
                return makeAdded<Self>(self, slot, false);
            if (not slot)
            {
                Slot& fresh = store.takeFreeSlot();
                try
                {
                    std::construct_at(fresh.body.entry.value(), id, ARC_FWD(sharedArgs)...);
                    try
                    {
                        store.index.emplace(id, fresh.index);
                    }
                    catch (...)
                    {
                        std::destroy_at(std::addressof(fresh.entry()));
                        throw;
                    }
                }
                catch (...)
                {
                    store.putFreeSlot(fresh);
                    throw;
                }
                fresh.linked = true;
                fresh.birth = ++store.births;
                slot = std::addressof(fresh);
            }

            Pin const pin(store, slot);
            Bitset const engagedBefore = slot->engaged;
            {
                InitScope const scope{store, *slot};
                forEachIndex<DataListenerCount>(
                    [&store, slot]<std::size_t I>(std::integral_constant<std::size_t, I>)
                    {
                        // Alive but not engaged: released, and pinned; offered again once destroyed
                        if (slot->alive.test(I))
                            return;
                        slot->initListener = static_cast<std::uint8_t>(I);
                        if constexpr (initReturnsVoid<I>())
                            store.init<I>(*slot);
                        else
                            store.emplacePrivateData<I>(*slot, [&] { return store.init<I>(*slot); });
                    });
            }
            if (not slot->linked)
                return makeAdded<Self>(self, nullptr, false);
            return makeAdded<Self>(self, slot, engagedBefore != slot->engaged);
        }

        template<class Self>
        [[nodiscard]] auto get(this Self& self, Id const& id) -> RefFor<Self>
        {
            return makeRef<Self>(self, self.find(id));
        }

        template<class Self>
        [[nodiscard]] auto resolve(this Self& self, Handle const& handle) -> RefFor<Self>
        {
            Node const& store = self;
            if (handle.slot() >= store.slots.size())
                return makeRef<Self>(self, nullptr);
            Slot& slot = const_cast<Slot&>(store.slots[handle.slot()]);
            return refIfLinked<Self>(self, slot, handle.generation());
        }

        template<class Self>
        [[nodiscard]] auto resolve(this Self& self, Ref const& ref) -> RefFor<Self>
        {
            return makeRef<Self>(self, self.slotOf(ref));
        }

        template<class Self, std::invocable<SharedData*, PrivateArg<Self>> UseData>
        auto modify(this Self& self, Id const& id, UseData&& f) -> RefFor<Self>
        {
            Slot* const slot = self.find(id);
            if (not slot)
                return makeRef<Self>(self, nullptr);
            std::uint32_t const generation = slot->generation;
            {
                Pin const pin(self, slot);
                useData(self, *slot, detail::UseDataSwallowId{ARC_FWD(f)});
            }
            return refIfLinked<Self>(self, *slot, generation);
        }

        template<class Self, IsDataRefFor<CallerTypes<Self>> R, std::invocable<SharedData*, PrivateArg<Self>> UseData>
        void modify(this Self& self, R const& ref, UseData&& f)
        {
            if (Slot* const slot = self.slotOf(ref))
                useData(self, *slot, detail::UseDataSwallowId{ARC_FWD(f)});
        }

        template<class Self, std::invocable<Id const&, detail::ConstLike<Self, SharedData>*, PrivateArg<Self>> UseData>
        void forEach(this Self& self, UseData&& f)
        {
            self.forEachSlot([&](Slot& slot) { useData(self, slot, f); });
        }

        template<class Self>
        auto notify(this Self& self, auto const& event, Id const& id) -> RefFor<Self>
        {
            Slot* const slot = self.find(id);
            if (not slot)
                return makeRef<Self>(self, nullptr);
            std::uint32_t const generation = slot->generation;
            {
                Pin const pin(self, slot);
                self.template notifySlot<noListener>(*slot, event);
            }
            return refIfLinked<Self>(self, *slot, generation);
        }

        template<class Self, IsDataRefFor<CallerTypes<Self>> R>
        void notify(this Self& self, auto const& event, R const& ref)
        {
            if (Slot* const slot = self.slotOf(ref))
                self.template notifySlot<noListener>(*slot, event);
        }

        void notifyAll(auto const& event)
        {
            forEachSlot([&](Slot& slot) { notifySlot<noListener>(slot, event); });
        }

        template<class Self, class... Args>
        requires IsListenerSelf<Self>
        auto subscribe(this Self& self, Id const& id, Args&&... args) -> RefFor<Self>
        {
            return self.subscribeSlot(self.find(id), ARC_FWD(args)...);
        }

        template<class Self, IsDataRefFor<CallerTypes<Self>> R, class... Args>
        requires IsListenerSelf<Self>
        auto subscribe(this Self& self, R const& ref, Args&&... args) -> RefFor<Self>
        {
            Slot* const slot = self.slotOf(ref);
            return self.subscribeSlot(slot and slot->linked ? slot : nullptr, ARC_FWD(args)...);
        }

        template<class Self>
        requires IsListenerSelf<Self>
        bool release(this Self& self, Id const& id)
        {
            return self.releaseSlot(self.find(id));
        }

        template<class Self, IsDataRefFor<CallerTypes<Self>> R>
        requires IsListenerSelf<Self>
        bool release(this Self& self, R const& ref)
        {
            return self.releaseSlot(self.slotOf(ref));
        }
    };
};

template<class SharedData, template<class Key, class Value, class...> class Map>
template<class Context>
template<std::size_t I>
struct DataStoreNode<SharedData, Map>::Node<Context>::FromDataListener : Node
{
    static constexpr std::size_t ListenerIndex = I;

    using Types = TypesFor<I>;
    using PrivateData = Types::PrivateData;
};

namespace node {
    ARC_MODULE_EXPORT
    template<class SharedData, template<class Key, class Value, class...> class MapTmpl = std::unordered_map>
    using DataStore = arc::DataStoreNode<SharedData, MapTmpl>;
}

} // namespace arc

#endif // INCLUDE_ARC_DATA_STORE_HPP
