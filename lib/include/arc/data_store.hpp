#ifndef INCLUDE_ARC_DATA_STORE_HPP
#define INCLUDE_ARC_DATA_STORE_HPP


#include "arc/detail/bitset.hpp"
#include "arc/detail/concepts.hpp"
#include "arc/detail/storage.hpp"
#include "arc/empty_types.hpp"
#include "arc/macros.hpp"
#include "arc/repeater.hpp"

#include "arc/traits/datastore.hxx"

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

        ~DataStoreHolder()
        {
            arc::forEachIndex<Count>(
                [this](auto i) {
                    if (engaged.test(i))
                        std::destroy_at(get<i>(privateData).value());
                });
        }

        constexpr DataStoreHolder(DataStoreHolder const& other) : engaged(other.engaged), sharedData(other.sharedData)
        {
            arc::forEachIndex<Count>(
                [this, &other](auto i)
                {
                    if (engaged.test(i))
                        std::construct_at(get<i>(privateData).storage(), *get<i>(other.privateData).value());
                });
        }

        constexpr DataStoreHolder(DataStoreHolder&& other) : engaged(other.engaged), sharedData(std::move(other.sharedData))
        {
            arc::forEachIndex<Count>(
                [this, &other](auto i)
                {
                    if (engaged.test(i))
                        std::construct_at(get<i>(privateData).storage(), std::move(*get<i>(other.privateData).value()));
                });
        }

        Bitset<Count> engaged;
        [[no_unique_address]] SharedData sharedData{};
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
template<class SharedData, template<class Key, class Value, class...> class Map = std::unordered_map>
struct DataStore
{
    using Id = SharedData::Id;
    using DataStoreTrait = trait::DataStore<SharedData>;
    using DataListenerTrait = trait::DataListener<SharedData>;

    template<class Context>
    struct Node : arc::NodeImpl<DataStoreTrait>
    {
        using Depends = arc::Depends<DataListenerTrait>;
        static constexpr std::size_t DataListenerCount = arc::ResolveTypes<Node, DataListenerTrait>::TypesCount;
        struct Types
        {
            using PrivateData = void;
        };

    private:
        template<std::size_t I>
        using PrivateDataAt = arc::ResolveTypes<Node, DataListenerTrait, key::RepeaterIndex<I>>::PrivateData;

        using IndexSequence = std::make_index_sequence<DataListenerCount>;

        template<std::size_t... Is>
        static auto getTypes(std::index_sequence<Is...>) -> detail::DataStoreHolder<SharedData, PrivateDataAt<Is>...>;

        using Data = decltype(getTypes(IndexSequence{}));

        Map<Id, Data> dataMap;

        template<std::size_t ListenerIndex>
        struct FromDataListener;

        template<class Listener, std::size_t... Is>
        static consteval std::size_t indexOfListener(std::index_sequence<Is...>)
        {
            using ListenerContext = ContextOf<Listener>;
            using RepeaterNode = detail::ResolveTraitFromNode<Node, DataListenerTrait>::Node;
            return arc::indexOf<ListenerContext, ContextOf<typename detail::ResolveTraitFromNode<RepeaterNode, RepeaterTrait<Is>>::Node>...>();
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
                forEachIndex<DataListenerCount>(
                    [&self, &item, &event](auto i)
                    {
                        if (item.second.engaged.test(i))
                            self.getNode(trait::dataListener<SharedData>, key::repeaterIndex<i>).onEvent(
                                detail::getEvent(event), item.first, item.second.sharedData, *get<i>(item.second.privateData).value());
                    });
            }
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
        std::pair<SharedData const*, bool> impl(DataStoreTrait::add, Id const& id, SharedArgs&&... sharedArgs)
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

                        using PrivateData = PrivateDataAt<I>;
                        auto const constructor = [&holder = it->second]<class... PrivateArgs>(PrivateArgs&&... args) -> PrivateData&
                            requires std::constructible_from<PrivateData, PrivateArgs...>
                        {
                            PrivateData* p = std::construct_at(get<I>(holder.privateData).storage(), ARC_FWD(args)...);
                            holder.engaged.set(I);
                            return *p;
                        };
                        this->getNode(trait::dataListener<SharedData>, key::repeaterIndex<I>).init(
                            it->first, it->second.sharedData, constructor);
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

        [[nodiscard]] ARC_INLINE constexpr auto impl(DataStoreTrait::get, Id const& id) const
            -> std::pair<SharedData const*, void*>
        {
            auto const it = dataMap.find(id);
            if (it != dataMap.end())
                return {std::addressof(it->second.sharedData), nullptr};
            return {nullptr, nullptr};
        }

        template<std::invocable<SharedData*, void*> UseData>
        constexpr auto impl(DataStoreTrait::modify, Id const& id, UseData&& f)
            -> std::pair<SharedData const*, void*>
        {
            if (auto const it = dataMap.find(id); it != dataMap.end())
            {
                useData(*it, detail::UseDataSwallowId{ARC_FWD(f)});
                return {std::addressof(it->second.sharedData), nullptr};
            }
            return {nullptr, nullptr};
        }

        template<class Self, std::invocable<Id const&, detail::ConstLike<Self, SharedData>*, void*> UseData>
        constexpr void impl(this Self& self, DataStoreTrait::forEach, UseData&& f)
        {
            for (auto& item : self.dataMap)
                self.useData(item, ARC_FWD(f));
        }

        std::pair<SharedData const*, void*> impl(DataStoreTrait::notify, auto const& tag, Id const& id)
        {
            auto const it = dataMap.find(id);
            if (it != dataMap.end())
            {
                forEachIndex<DataListenerCount>(
                    [this, &tag, it](auto i)
                    {
                        if (it->second.engaged.test(i))
                            this->getNode(trait::dataListener<SharedData>, key::repeaterIndex<i>).onEvent(
                                tag, it->first, it->second.sharedData, *get<i>(it->second.privateData).value());
                    });
                return {std::addressof(it->second.sharedData), nullptr};
            }
            return {nullptr, nullptr};
        }

        void impl(DataStoreTrait::notifyAll, auto const& tag)
        {
            for (auto& item : dataMap)
            {
                forEachIndex<DataListenerCount>(
                    [this, &tag, &item](auto i)
                    {
                        if (item.second.engaged.test(i))
                            this->getNode(trait::dataListener<SharedData>, key::repeaterIndex<i>).onEvent(
                                tag, item.first, item.second.sharedData, *get<i>(item.second.privateData).value());
                    });
            }
        }
    };
};

template<class SharedData, template<class Key, class Value, class...> class Map>
template<class Context>
template<std::size_t ListenerIndex>
struct DataStore<SharedData, Map>::Node<Context>::FromDataListener : Node
{
    struct Types
    {
        using PrivateData = PrivateDataAt<ListenerIndex>;
    };
    using PrivateData = Types::PrivateData;

    template<class Self, class UseData>
    void useData(this Self& self, auto& item, UseData&& f)
    {
        detail::ConstLike<Self, SharedData>* sharedData = std::addressof(item.second.sharedData);
        detail::ConstLike<Self, PrivateData>* privateData = item.second.engaged.test(ListenerIndex)
            ? get<ListenerIndex>(item.second.privateData).value()
            : nullptr;
        std::same_as<Id> auto const& id = item.first;

        if constexpr (detail::IsNotEvent<std::invoke_result_t<UseData, Id const&, decltype(sharedData), decltype(privateData)>>)
        {
            // No event: shared data is const
            f(id, static_cast<SharedData const*>(sharedData), privateData);
        }
        else if (decltype(auto) event = f(id, sharedData, privateData); detail::isEvent(event))
        {
            static_assert(not std::is_const_v<Self>, "useData with returned event cannot be called on const DataStore");
            forEachIndex<DataListenerCount>(
                [&self, &item, &event](auto i)
                {
                    if constexpr (i != ListenerIndex)
                        if (item.second.engaged.test(i))
                            self.getNode(trait::dataListener<SharedData>, key::repeaterIndex<i>).onEvent(
                                detail::getEvent(event), item.first, item.second.sharedData, *get<i>(item.second.privateData).value());
                });
        }
    }

    template<class Self>
    [[nodiscard]] constexpr auto impl(this Self& self, DataStoreTrait::get, Id const& id)
        -> std::pair<SharedData const*, detail::ConstLike<Self, PrivateData>*>
    {
        auto const it = self.dataMap.find(id);
        if (it != self.dataMap.end())
        {
            if (it->second.engaged.test(ListenerIndex))
                return {std::addressof(it->second.sharedData), get<ListenerIndex>(it->second.privateData).value()};
            else
                return {std::addressof(it->second.sharedData), nullptr};
        }
        return {nullptr, nullptr};
    }

    template<std::invocable<SharedData*, PrivateData*> UseData>
    constexpr auto impl(DataStoreTrait::modify, Id const& id, UseData&& f)
        -> std::pair<SharedData const*, PrivateData*>
    {
        if (auto const it = dataMap.find(id); it != dataMap.end())
        {
            useData(*it, detail::UseDataSwallowId{ARC_FWD(f)});
            return {std::addressof(it->second.sharedData), std::addressof(*get<ListenerIndex>(it->second.privateData).value())};
        }
        return {nullptr, nullptr};
    }

    template<class Self, std::invocable<Id const&, detail::ConstLike<Self, SharedData>*, detail::ConstLike<Self, PrivateData>*> UseData>
    constexpr void impl(this Self& self, DataStoreTrait::forEach, UseData&& f)
    {
        for (auto& item : self.dataMap)
            self.useData(item, ARC_FWD(f));
    }

    std::pair<SharedData const*, PrivateData*> impl(DataStoreTrait::notify, auto const& tag, Id const& id)
    {
        if (auto const it = dataMap.find(id); it != dataMap.end())
        {
            forEachIndex<DataListenerCount>(
                [this, &tag, it](auto i)
                {
                    if (it->second.engaged.test(i))
                        this->getNode(trait::dataListener<SharedData>, key::repeaterIndex<i>).onEvent(
                            tag, it->first, it->second.sharedData, *get<i>(it->second.privateData).value());
                });
            return {std::addressof(it->second.sharedData), get<ListenerIndex>(it->second.privateData).value()};
        }
        return {nullptr, nullptr};
    }
};

} // namespace arc


#endif // INCLUDE_ARC_DATA_STORE_HPP
