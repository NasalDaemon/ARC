#ifndef INCLUDE_ARC_DATA_STORE_HPP
#define INCLUDE_ARC_DATA_STORE_HPP


#include "arc/detail/bitset.hpp"
#include "arc/detail/concepts.hpp"
#include "arc/detail/storage.hpp"
#include "arc/empty_types.hpp"
#include "arc/macros.hpp"
#include "arc/nodes/repeater.hpp"

#include "arc/traits/datastore.hpp"
#include "arc/detail/with_index.hpp"

#if !ARC_IMPORT_STD
#include <cstddef>
#include <memory>
#include <optional>
#include <type_traits>
#include <unordered_map>
#include <utility>
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

    template<class SharedData, class... Data>
    struct DataStoreHolder
    {
        static constexpr std::size_t Count = sizeof...(Data);

        constexpr explicit DataStoreHolder(auto&&... sharedDataArgs)
            : sharedData{ARC_FWD(sharedDataArgs)...}
        {}

        constexpr DataStoreHolder(DataStoreHolder const& other) : eventId(other.eventId), sharedData(other.sharedData)
        {
            arc::forEachIndex<Count>(
                [this, &other](auto i)
                {
                    if (other.engaged.test(i))
                    {
                        std::construct_at(std::get<i>(privateData).storage(), *std::get<i>(other.privateData).value());
                        engaged.set(i);
                    }
                });
        }

        constexpr DataStoreHolder(DataStoreHolder&& other) : eventId(other.eventId), sharedData(std::move(other.sharedData))
        {
            arc::forEachIndex<Count>(
                [this, &other](auto i)
                {
                    if (other.engaged.test(i))
                    {
                        std::construct_at(std::get<i>(privateData).storage(), std::move(*std::get<i>(other.privateData).value()));
                        engaged.set(i);
                    }
                });
        }

        constexpr ~DataStoreHolder()
        {
            arc::forEachIndex<Count>(
                [this](auto i) {
                    if (engaged.test(i))
                        std::destroy_at(std::get<i>(privateData).value());
                });
        }

        std::size_t eventId = 0;
        Bitset<Count> engaged;
        [[no_unique_address]] SharedData sharedData;
        [[no_unique_address]] std::tuple<detail::Storage<Data>...> privateData;
    };

    template<class T>
    constexpr bool isEvent(std::optional<T> const& event)
    {
        return event.has_value();
    }
    template<class T>
    constexpr std::true_type isEvent(T const&)
    {
        static_assert(not IsNotEvent<T>);
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
        static_assert(not IsNotEvent<T>);
        return event;
    }

} // namespace detail

ARC_MODULE_EXPORT
template<class SharedData, template<class Key, class Value, class...> class MapTmpl = std::unordered_map>
struct DataStoreNode
{
    using Id = SharedData::Id;
    using DataStoreTrait = trait::DataStore<SharedData>;
    using DataListenerTrait = trait::DataListener<SharedData>;

    template<class Context>
    struct Node : arc::NodeImpl<DataStoreTrait>
    {
        using Depends = arc::Depends<DataListenerTrait>;

        struct Types
        {
            using PrivateData = void;
        };

    private:
        static consteval std::size_t dataListenerCount()
        {
            using ResolvedTypes = arc::ResolveTypes<Node, DataListenerTrait>;
            if constexpr (requires { ResolvedTypes::TypesCount; })
            {
                static_assert(ResolvedTypes::TypesCount > 1, "DataStore behind a repeater requires at least two DataListeners");
                return ResolvedTypes::TypesCount;
            }
            else
            {
                return 1;
            }
        }

        static constexpr std::size_t DataListenerCount = dataListenerCount();

        template<std::size_t I>
        using PrivateDataAt = arc::ResolveTypes<Node, DataListenerTrait, key::RepeaterIndex<I>>::PrivateData;

        using IndexSequence = std::make_index_sequence<DataListenerCount>;

        template<std::size_t... Is>
        static auto getTypes(std::index_sequence<Is...>) -> detail::DataStoreHolder<SharedData, PrivateDataAt<Is>...>;

        using Data = decltype(getTypes(IndexSequence{}));
        using Bitset = decltype(Data::engaged);
        using Map = MapTmpl<Id, Data>;
        using Item = Map::value_type;
        Map dataMap;

        template<std::size_t ListenerIndex>
        struct FromDataListener;

        template<class Listener, std::size_t... Is>
        static consteval std::size_t indexOfListener(std::index_sequence<Is...>)
        {
            using ListenerContext = ContextOf<Listener>;
            using ResolvedNode = detail::ResolveTraitFromNode<Node, DataListenerTrait>::Node;
            if constexpr (sizeof...(Is) == 1)
            {
                static_assert(std::is_same_v<ListenerContext, ContextOf<ResolvedNode>>);
                return 0;
            }
            else
            {
                return arc::indexOf<ListenerContext, ContextOf<typename detail::ResolveTraitFromNode<ResolvedNode, RepeaterTrait<Is>>::Node>...>();
            }
        }

        template<std::size_t I>
        constexpr auto getDataListener(this auto& self, std::integral_constant<std::size_t, I> = {})
        {
            if constexpr (DataListenerCount == 1)
                return self.getNode(DataListenerTrait{});
            else
                return self.getNode(DataListenerTrait{}, key::repeaterIndex<I>);
        }

        template<HasTrait<DataListenerTrait> Source>
        static auto finaliseTypes() -> FromDataListener<indexOfListener<Source>(IndexSequence{})>::Types;
        template<class Source>
        static auto finaliseTypes() -> Types;

        template<class Self, class UseData>
        void useData(this Self& self, auto& item, UseData&& f)
        {
            detail::ConstLike<Self, SharedData>* sharedData = std::addressof(item.second.sharedData);
            std::same_as<Id> auto const& id = item.first;
            if constexpr (detail::IsNotEvent<std::invoke_result_t<UseData, Id const&, decltype(sharedData), void*>>)
            {
                // No event: shared data is const
                f(id, static_cast<SharedData const*>(sharedData), static_cast<void*>(nullptr));
            }
            else if (decltype(auto) event = f(id, sharedData, static_cast<void*>(nullptr)); detail::isEvent(event))
            {
                static_assert(not std::is_const_v<Self>, "useData with returned event cannot be called on const DataStore");
                self.notifyItem(item, detail::getEvent(event));
            }
        }

        bool notifyItem(Item& item, auto const& event)
        {
            ++item.second.eventId;
            return forAllIndices<DataListenerCount>(
                [this, event = Event{std::addressof(event), &item.second.eventId}, &item](auto i)
                {
                    if (isEngaged<i>(item.second.engaged))
                    {
                        auto listener = this->getDataListener(i);
                        auto& privateData = *std::get<i>(item.second.privateData).value();
                        using EventView = EventItem<SharedData const, PrivateDataAt<i> const>;
                        if constexpr (requires { listener.skipEvent(event, EventView{item.first, item.second.sharedData, privateData}); })
                            if (listener.skipEvent(event, EventView{item.first, item.second.sharedData, privateData}))
                                return true;
                        using result_t = decltype(listener.onEvent(event, EventItem{item.first, item.second.sharedData, privateData}));
                        if constexpr (std::is_void_v<result_t>)
                            listener.onEvent(event, EventItem{item.first, item.second.sharedData, privateData});
                        else
                            return listener.onEvent(event, EventItem{item.first, item.second.sharedData, privateData});
                    }
                    return true;
                });
        }

        template<class Self>
        detail::ConstLike<Self, Data>& getHolder(this Self&, SharedData const* sharedDataPtr)
        {
            return ARC_MEM_PTR(Data, sharedData).getClassFromMember(*const_cast<SharedData*>(sharedDataPtr));
        }
        template<class Self>
        detail::ConstLike<Self, Item>& getItem(this Self& self, SharedData const* sharedDataPtr)
        {
            return ARC_MEM_PTR(Item, second).getClassFromMember(self.getHolder(sharedDataPtr));
        }

        template<std::size_t I>
        constexpr decltype(auto) init(Item& item)
        {
            using PrivateData = PrivateDataAt<I>;
            auto const constructor = [&holder = item.second]<class... Args>(Args&&... args) -> PrivateData*
                requires std::constructible_from<PrivateData, Args...>
            {
                static_assert(detail::alwaysTrue<Args...> and isOptionalListener<I>(), "DataListener init that returns PrivateData must not use the injected constructor");
                PrivateData* p = std::construct_at(std::get<I>(holder.privateData).storage(), ARC_FWD(args)...);
                holder.engaged.set(I);
                return p;
            };
            return this->getDataListener<I>().init(item.first, item.second.sharedData, constructor);
        }

        template<std::size_t I>
        static constexpr auto isEngaged(Bitset const& bitset)
        {
            if constexpr (isOptionalListener<I>())
                return bitset.test(I);
            else
                return std::true_type{};
        }

        template<std::size_t I>
        static consteval bool isOptionalListener()
        {
            return std::is_void_v<decltype(std::declval<Node>().template init<I>(std::declval<Item&>()))>;
        }

    public:
        Node() = default;

        template<class Source, class Self, class Types, class... Keys>
        using FinaliseTypes = decltype(finaliseTypes<Source>());

        template<HasTrait<DataListenerTrait> Source, class Key = ContextOf<Source>::Info::DefaultKey>
        ARC_INLINE constexpr auto finalise(this auto& self, Source& source, Key const& key = {}, auto const&... keys)
        {
            static constexpr std::size_t index = indexOfListener<Source>(IndexSequence{});
            return Context::Info::finalise(source, detail::downCast<FromDataListener<index>>(self), key, keys...);
        }
        template<class Source, class Key = ContextOf<Source>::Info::DefaultKey>
        ARC_INLINE constexpr auto finalise(this auto& self, Source& source, Key const& key = {}, auto const&... keys)
        {
            return Context::Info::finalise(source, self, key, keys...);
        }

        template<class... SharedArgs>
        requires std::constructible_from<SharedData, SharedArgs...>
        std::pair<SharedData const*, bool> add(Id const& id, SharedArgs&&... sharedArgs)
        {
            auto [it, inserted] = dataMap.try_emplace(id, ARC_FWD(sharedArgs)...);
            if (not it->second.engaged.all())
            {
                auto const oldEngaged = it->second.engaged;
                forEachIndex<DataListenerCount>(
                    [this, it]<std::size_t I>(std::integral_constant<std::size_t, I>)
                    {
                        if (it->second.engaged.test(I))
                            return;

                        if constexpr (isOptionalListener<I>())
                        {
                            this->init<I>(*it);
                        }
                        else
                        {
                            new (std::get<I>(it->second.privateData).storage()) PrivateDataAt<I>(this->init<I>(*it));
                            it->second.engaged.set(I);
                        }
                    });
                if (it->second.engaged.none())
                {
                    dataMap.erase(it);
                    return {nullptr, false};
                }
                inserted = oldEngaged != it->second.engaged;
            }
            return {std::addressof(it->second.sharedData), inserted};
        }

        [[nodiscard]] ARC_INLINE constexpr auto get(Id const& id) const
            -> std::pair<SharedData const*, void*>
        {
            auto const it = dataMap.find(id);
            if (it != dataMap.end())
                return {std::addressof(it->second.sharedData), nullptr};
            return {nullptr, nullptr};
        }
        [[nodiscard]] ARC_INLINE constexpr void* get(SharedData const*) const
        {
            return nullptr;
        }

        template<std::invocable<SharedData*, void*> UseData>
        constexpr auto modify(Id const& id, UseData&& f)
            -> std::pair<SharedData const*, void*>
        {
            if (auto const it = dataMap.find(id); it != dataMap.end())
            {
                useData(*it, detail::UseDataSwallowId{ARC_FWD(f)});
                return {std::addressof(it->second.sharedData), nullptr};
            }
            return {nullptr, nullptr};
        }
        template<std::invocable<SharedData*, void*> UseData>
        constexpr void* modify(SharedData const* sharedData, UseData&& f)
        {
            auto& item = getItem(sharedData);
            useData(item, detail::UseDataSwallowId{ARC_FWD(f)});
            return nullptr;
        }

        template<class Self, std::invocable<Id const&, detail::ConstLike<Self, SharedData>*, void*> UseData>
        constexpr void forEach(this Self& self, UseData&& f)
        {
            for (auto& item : self.dataMap)
                self.useData(item, ARC_FWD(f));
        }

        std::pair<SharedData const*, void*> notify(auto const& tag, Id const& id)
        {
            auto const it = dataMap.find(id);
            if (it != dataMap.end())
            {
                notifyItem(*it, tag);
                return {std::addressof(it->second.sharedData), nullptr};
            }
            return {nullptr, nullptr};
        }

        void* notify(auto const& tag, SharedData const* sharedData)
        {
            notifyItem(getItem(sharedData), tag);
            return nullptr;
        }

        void notifyAll(auto const& tag)
        {
            for (auto& item : dataMap)
                notifyItem(item, tag);
        }
    };
};

template<class SharedData, template<class Key, class Value, class...> class Map>
template<class Context>
template<std::size_t ListenerIndex>
struct DataStoreNode<SharedData, Map>::Node<Context>::FromDataListener : Node
{
    struct Types
    {
        using PrivateData = PrivateDataAt<ListenerIndex>;
    };
    using PrivateData = Types::PrivateData;

    static auto* getPrivateData(auto& holder)
    {
        return isEngaged<ListenerIndex>(holder.engaged) ? std::get<ListenerIndex>(holder.privateData).value() : nullptr;
    }

    template<class Self, class UseData>
    void useData(this Self& self, Item& item, UseData&& f)
    {
        auto const& id = item.first;
        detail::ConstLike<Self, SharedData>* sharedData = std::addressof(item.second.sharedData);
        detail::ConstLike<Self, PrivateData>* privateData = getPrivateData(item.second);

        if constexpr (detail::IsNotEvent<std::invoke_result_t<UseData, Id const&, decltype(sharedData), decltype(privateData)>>)
        {
            // No event: shared data is const
            f(id, static_cast<SharedData const*>(sharedData), privateData);
        }
        else if (decltype(auto) event = f(id, sharedData, privateData); detail::isEvent(event))
        {
            static_assert(not std::is_const_v<Self>, "useData with returned event cannot be called on const DataStore");
            self.template notifyItem<true>(item, detail::getEvent(event));
        }
    }

    template<bool SkipSelf>
    bool notifyItem(Item& item, auto const& event)
    {
        ++item.second.eventId;
        return forAllIndices<DataListenerCount>(
            [this, event = Event{std::addressof(event), &item.second.eventId}, &item](auto i)
            {
                if constexpr (not SkipSelf or i != ListenerIndex)
                {
                    if (isEngaged<i>(item.second.engaged))
                    {
                        auto listener = this->getDataListener(i);
                        auto& privateData = *std::get<i>(item.second.privateData).value();
                        using EventView = EventItem<SharedData const, PrivateData const>;
                        if constexpr (requires { listener.skipEvent(event, EventView{item.first, item.second.sharedData, privateData}); })
                            if (listener.skipEvent(event, EventView{item.first, item.second.sharedData, privateData}))
                                return true;
                        using result_t = decltype(listener.onEvent(event, EventItem{item.first, item.second.sharedData, privateData}));
                        if constexpr (std::is_void_v<result_t>)
                            listener.onEvent(event, EventItem{item.first, item.second.sharedData, privateData});
                        else
                            return listener.onEvent(event, EventItem{item.first, item.second.sharedData, privateData});
                    }
                }
                return true;
            });
    }

    template<class Self>
    [[nodiscard]] constexpr auto get(this Self& self, Id const& id)
        -> std::pair<SharedData const*, detail::ConstLike<Self, PrivateData>*>
    {
        auto const it = self.dataMap.find(id);
        if (it != self.dataMap.end())
            return {std::addressof(it->second.sharedData), getPrivateData(it->second)};
        return {nullptr, nullptr};
    }

    template<class Self>
    [[nodiscard]] constexpr auto get(this Self& self, SharedData const* sharedData)
        -> detail::ConstLike<Self, PrivateData>*
    {
        return getPrivateData(self.getHolder(sharedData));
    }

    template<std::invocable<SharedData*, PrivateData*> UseData>
    constexpr auto modify(Id const& id, UseData&& f)
        -> std::pair<SharedData const*, PrivateData*>
    {
        if (auto const it = dataMap.find(id); it != dataMap.end())
        {
            useData(*it, detail::UseDataSwallowId{ARC_FWD(f)});
            return {std::addressof(it->second.sharedData), getPrivateData(it->second)};
        }
        return {nullptr, nullptr};
    }

    template<std::invocable<SharedData*, PrivateData*> UseData>
    constexpr auto modify(SharedData const* sharedData, UseData&& f)
        -> PrivateData*
    {
        auto& item = getItem(sharedData);
        useData(item, detail::UseDataSwallowId{ARC_FWD(f)});
        return getPrivateData(item.second);
    }

    template<class Self, std::invocable<Id const&, detail::ConstLike<Self, SharedData>*, detail::ConstLike<Self, PrivateData>*> UseData>
    constexpr void forEach(this Self& self, UseData&& f)
    {
        for (auto& item : self.dataMap)
            self.useData(item, ARC_FWD(f));
    }

    std::pair<SharedData const*, PrivateData*> notify(auto const& tag, Id const& id)
    {
        if (auto const it = dataMap.find(id); it != dataMap.end())
        {
            notifyItem<false>(*it, tag);
            return {std::addressof(it->second.sharedData), getPrivateData(it->second)};
        }
        return {nullptr, nullptr};
    }

    PrivateData* notify(auto const& tag, SharedData const* sharedData)
    {
        auto& item = getItem(sharedData);
        notifyItem<false>(item, tag);
        return getPrivateData(item.second);
    }
};

namespace node {
    ARC_MODULE_EXPORT
    template<class SharedData, template<class Key, class Value, class...> class MapTmpl = std::unordered_map>
    using DataStore = arc::DataStoreNode<SharedData, MapTmpl>;
}

} // namespace arc

#endif // INCLUDE_ARC_DATA_STORE_HPP
