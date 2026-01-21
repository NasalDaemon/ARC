#ifndef INCLUDE_ARC_DATA_STORE_HPP
#define INCLUDE_ARC_DATA_STORE_HPP


#include "arc/detail/storage.hpp"
#include "arc/bitset.hpp"
#include "arc/macros.hpp"

#include "arc/traits/datastore.hxx"

#if !ARC_IMPORT_STD
#include <memory>
#include <unordered_map>
#include <utility>
#endif

namespace arc {

namespace detail {

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
        std::tuple<detail::Storage<Data>...> privateData;
    };

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

    private:
        template<std::size_t I>
        using PrivateDataAt = arc::ResolveTypes<Node, DataListenerTrait>::template TypesAt<I>::PrivateData;

        using IndexSequence = std::make_index_sequence<DataListenerCount>;

        template<std::size_t... Is>
        static auto getTypes(std::index_sequence<Is...>) -> detail::DataStoreHolder<SharedData, PrivateDataAt<Is>...>;

        using Data = decltype(getTypes(IndexSequence{}));

        Map<Id, Data> dataMap;

        template<class SourceData>
        struct FromDataListener;

    public:
        Node() = default;

        template<class Source, class Key = ContextOf<Source>::Info::DefaultKey>
        ARC_INLINE constexpr auto finalise(this auto& self, Source& source, Key const& key = {}, auto const&... keys)
        {
            if constexpr (HasTrait<Source, DataListenerTrait>)
            {
                using SourceData = Source::Traits::template ResolveTypes<DataListenerTrait, Source>::PrivateData;
                return Context::Info::finalise(source, detail::downCast<FromDataListener<SourceData>>(self), key, keys...);
            }
            else
            {
                return Context::Info::finalise(source, self, key, keys...);
            }
        }

        bool impl(DataStoreTrait::add, Id const& id, auto&&... dataArgs)
        {
            auto const [it, inserted] = dataMap.try_emplace(id, ARC_FWD(dataArgs)...);
            if (inserted)
            {
                forEachIndex<DataListenerCount>(
                    [p = this, &id, it]<std::size_t I>(std::integral_constant<std::size_t, I>)
                    {
                        using PrivateData = PrivateDataAt<I>;
                        auto const constructor = [&holder = it->second]<class... Args>(Args&&... args) -> PrivateData&
                            requires std::constructible_from<PrivateData, Args...>
                        {
                            PrivateData* p = std::construct_at(get<I>(holder.privateData).storage(), ARC_FWD(args)...);
                            holder.engaged.set(I);
                            return *p;
                        };
                        p->getNode(trait::dataListener<SharedData>, key::repeaterIndex<I>).init(
                            id, it->second.sharedData, constructor);
                    });
                if (it->second.engaged.any())
                    return true;
                dataMap.erase(it);
            }
            return false;
        }

        ARC_INLINE constexpr auto impl(DataStoreTrait::get, Id const& id) const
            -> std::pair<SharedData const*, void const*>
        {
            auto const it = dataMap.find(id);
            if (it != dataMap.end())
                return {std::addressof(it->second.sharedData), nullptr};
            return {nullptr, nullptr};
        }

        SharedData const* impl(DataStoreTrait::notify, auto const& tag, Id const& id)
        {
            auto const it = dataMap.find(id);
            if (it != dataMap.end())
            {
                forEachIndex<DataListenerCount>(
                    [p = this, &tag, &id, it](auto i)
                    {
                        if (it->second.engaged.test(i))
                            p->getNode(trait::dataListener<SharedData>, key::repeaterIndex<i>).onEvent(
                                tag, id, it->second.sharedData, *get<i>(it->second.privateData).value());
                    });
                return std::addressof(it->second.sharedData);
            }
            return nullptr;
        }

        void impl(DataStoreTrait::notifyAll, auto const& tag)
        {
            for (auto& [id, data] : dataMap)
            {
                forEachIndex<DataListenerCount>(
                    [p = this, &tag, &id, &data](auto i)
                    {
                        if (data.engaged.test(i))
                            p->getNode(trait::dataListener<SharedData>, key::repeaterIndex<i>).onEvent(
                                tag, id, data.sharedData, *get<i>(data.privateData).value());
                    });
            }
        }
    };
};

template<class SharedData, template<class Key, class Value, class...> class Map>
template<class Context>
template<class SourceData>
struct DataStore<SharedData, Map>::Node<Context>::FromDataListener : Node
{
    template<class Self>
    ARC_INLINE constexpr auto impl(this Self& self, DataStoreTrait::get, Id const& id)
        -> std::conditional_t<
            std::is_const_v<Self>,
            std::pair<SharedData const*, SourceData const*>,
            std::pair<SharedData*, SourceData*>
        >
    {
        auto const it = self.dataMap.find(id);
        if (it != self.dataMap.end())
        {
            static constexpr auto index = []<std::size_t... Is>(std::index_sequence<Is...>) {
                return arc::indexOf<SourceData, PrivateDataAt<Is>...>();
            }(IndexSequence{});

            if (it->second.engaged.test(index))
                return {std::addressof(it->second.sharedData), get<index>(it->second.privateData).value()};
            else
                return {std::addressof(it->second.sharedData), nullptr};
        }
        return {nullptr, nullptr};
    }
};

} // namespace arc


#endif // INCLUDE_ARC_DATA_STORE_HPP
