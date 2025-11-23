#ifndef INCLUDE_ARC_MOCK_HPP
#define INCLUDE_ARC_MOCK_HPP

#include "arc/circular_buffer.hpp"
#include "arc/context_fwd.hpp"
#include "arc/detail/type_name.hpp"
#include "arc/empty_types.hpp"
#include "arc/function.hpp"
#include "arc/macros.hpp"
#include "arc/mock_fwd.hpp"
#include "arc/node.hpp"
#include "arc/test_context.hpp"
#include "arc/test.hpp"
#include "arc/trait.hpp"
#include "arc/traits.hpp"
#include "arc/type_id.hpp"

#if !ARC_IMPORT_STD
#include <any>
#include <cstddef>
#include <iterator>
#include <optional>
#include <string>
#include <stdexcept>
#include <type_traits>
#include <unordered_map>
#include <variant>
#endif

namespace arc::test {

ARC_MODULE_EXPORT
enum class MockDefault
{
    ReturnDefault,
    ThrowIfMissing,
};

ARC_MODULE_EXPORT
struct MockParams
{
    MockDefault defaultBehaviour = MockDefault::ReturnDefault;
    bool counting = false;
    bool logAllCalls = false;
    std::size_t logBufferMaxSize = 1024;

    auto operator<=>(MockParams const&) const = default;
};

ARC_MODULE_EXPORT
struct ArgsTuple
{
    constexpr auto operator()(auto const&... args) const noexcept
    {
        return std::tuple(args...);
    };
};

ARC_MODULE_EXPORT
inline constexpr ArgsTuple argsTuple{};

namespace detail {

    struct MockReturn
    {
        template<class T>
        constexpr operator T&() const &&
        {
            if (Ref const* ref = std::get_if<Ref>(&value))
            {
                if (ref->convertibleTo<T&>())
                    return *static_cast<T*>(ref->ptr);
            }
            throw std::bad_any_cast();
        }

        template<class T>
        constexpr operator T&() &
        {
            if (std::any* any = std::get_if<std::any>(&value))
            {
                if (T* p = std::any_cast<T>(any))
                    return *p;
            }
            else if (Ref* ref = std::get_if<Ref>(&value))
            {
                if (ref->convertibleTo<T&>())
                    return *static_cast<T*>(ref->ptr);
            }
            else if (returnDefault)
            {
                return value.emplace<std::any>().emplace<T>();
            }
            throw std::bad_any_cast();
        }

        template<class T>
        constexpr operator T&&() &&
        {
            if (std::any* any = std::get_if<std::any>(&value))
            {
                if (T* p = std::any_cast<T>(any))
                    return std::forward<T>(*p);
            }
            else if (Ref* ref = std::get_if<Ref>(&value))
            {
                if (ref->convertibleTo<T&&>())
                    return std::forward<T>(*static_cast<T*>(ref->ptr));
            }
            else if (returnDefault)
            {
                return std::forward<T>(value.emplace<std::any>().emplace<std::remove_cvref_t<T>>());
            }
            throw std::bad_any_cast();
        }

        template<class T>
        constexpr void emplace(auto&& arg)
        {
            if constexpr (std::is_reference_v<T>)
                value.emplace<Ref>(std::addressof(arg), TypeId::of<std::remove_cvref_t<T>>(), std::is_lvalue_reference_v<T>, std::is_const_v<std::remove_reference_t<T>>);
            else
                value.emplace<std::any>(std::in_place_type<T>, ARC_FWD(arg));
        }

        constexpr void reset() { value.emplace<std::monostate>(); }

        constexpr void setReturnDefault() { returnDefault = true; }

    private:
        bool returnDefault = false;

        struct Ref
        {
            void* ptr = nullptr;
            TypeId typeId;
            bool lref = false;
            bool isConst = false;

            template<class T>
            constexpr bool convertibleTo() const
            {
                static_assert(std::is_reference_v<T>, "T must be a reference type");
                return lref == std::is_lvalue_reference_v<T>
                   and (std::is_const_v<std::remove_reference_t<T>> or not isConst)
                   and typeId == TypeId::of<std::remove_cvref_t<T>>() ;
            }
        };
        std::variant<std::monostate, std::any, Ref> value;
    };

    using UniversalFn = arc::Function<void(void* result, void** args), arc::FunctionPolicy{.copyable = true, .mutableCall = true, .constCall = false}>;
    struct MockDefs
    {
        UniversalFn con, mut;
    };

    struct MockBase : arc::test::TestOnlyNode
    {
        explicit MockBase(MockParams params = {})
            : defaultBehaviour(params.defaultBehaviour)
            , counting(params.counting)
            , loggingAllCalls(params.logAllCalls)
            , callLog(params.logBufferMaxSize)
        {}

        constexpr void setReturnDefault() { defaultBehaviour = MockDefault::ReturnDefault; }
        constexpr void setThrowIfMissing() { defaultBehaviour = MockDefault::ThrowIfMissing; }
        constexpr bool returnsDefault() const { return defaultBehaviour == MockDefault::ReturnDefault; }
        constexpr bool throwsIfMissing() const { return defaultBehaviour == MockDefault::ThrowIfMissing; }

        constexpr void enableCallCounting()
        {
            counting = true;
        }
        constexpr void disableCallCounting()
        {
            counting = false;
            loggingAllCalls = false;
        }
        constexpr void logAllCalls(bool enable = true, std::optional<std::size_t> newCapacity = std::nullopt)
        {
            if (enable)
            {
                if (newCapacity.has_value())
                    callLog.set_max_size(*newCapacity);
                callLog.reserve();
                if (loggingAllCalls)
                    return;
                if (callCount > 0)
                    throw std::runtime_error(
                        "logAllCalls: Cannot enable call logging after some calls have already "
                        "been counted in the mock, please reset tracking first");
                counting = true;
                loggingAllCalls = true;
            }
            else
            {
                loggingAllCalls = false;
            }
        }
        constexpr bool callCountingEnabled() const { return counting; }
        constexpr bool callLoggingEnabled() const { return loggingAllCalls; }

        void resetTracking()
        {
            callCount = 0;
            for (auto& [_, tracker] : trackerMap)
            {
                tracker.callCount = 0;
                if (tracker.logIndices)
                    tracker.logIndices->clear();
            }
            callLog.clear();
            ++trackingVersion;
        }

        void resetImpls()
        {
            impls.clear();
        }

        void resetTrackingAndImpls()
        {
            resetTracking();
            resetImpls();
        }

        void reinitialise(MockParams params = {})
        {
            resetTrackingAndImpls();
            defaultBehaviour = params.defaultBehaviour;
            counting = params.counting;
            loggingAllCalls = params.logAllCalls;
            callLog.set_max_size(params.logBufferMaxSize);
        }

        std::size_t totalCallCount() const
        {
            if (not counting)
                throw std::runtime_error("totalCallCount: Call counting is not enabled for this mock");
            return callCount;
        }
        template<IsTrait Trait>
        std::size_t traitCallCount(Trait = {}) const
        {
            if (not counting)
                throw std::runtime_error("traitCallCount: Call counting is not enabled for this mock");
            auto const it = trackerMap.find(traitType<Trait>());
            return it != trackerMap.end() ? it->second.callCount : 0ul;
        }
        template<class Method>
        std::size_t methodCallCount(Method = {}) const
        {
            if (not counting)
                throw std::runtime_error("methodCallCount: Call counting is not enabled for this mock");
            auto const it = trackerMap.find(methodType<Method>());
            return it != trackerMap.end() ? it->second.callCount : 0ul;
        }
        template<class Method, class... Args>
        std::size_t implCallCount() const
        {
            if (not counting)
                throw std::runtime_error("implCallCount: Call counting is not enabled for this mock");
            auto const it = trackerMap.find(implType<Method, Args...>());
            return it != trackerMap.end() ? it->second.callCount : 0ul;
        }

        template<class Method, class T>
        void methodReturns(T&& value)
        {
            auto const methodType_ = methodType<Method>();
            returnsMap[methodType_].template emplace<T>(ARC_FWD(value));
        }
        template<class Method, class... Args, class T>
        void implReturns(T&& value)
        {
            auto const implType_ = implType<Method, Args...>();
            returnsMap[implType_].template emplace<T>(ARC_FWD(value));
            impls.erase(implType_);
        }

        template<class Method, class... Args, std::invocable<std::remove_cvref_t<Args> const&...> F = ArgsTuple>
        auto visitCallLogs(F&& visitor = {})
        {
            return visitCallLogs<Method, Args...>(0, ARC_FWD(visitor));
        }

        template<class Method, class... Args, std::invocable<std::remove_cvref_t<Args> const&...> F = ArgsTuple>
        auto visitCallLogs(std::size_t startIndex, F&& visitor = {})
        {
            static_assert(sizeof...(Args) > 0, "Cannot visit last call with no arguments");
            static_assert((std::is_copy_constructible_v<std::remove_cvref_t<Args>> and ...),
                "All argument types must be copy constructible to be logged");
            if (not loggingAllCalls)
                throw std::runtime_error("visitCallLogs: Call logging is not enabled for this mock, please enable logging, or use subscribeCalls instead");

            using result_t = std::invoke_result_t<F, std::remove_cvref_t<Args> const&...>;
            auto const implType_ = implType<Method, Args...>();
            return CallLogVisitor<result_t>(*this, trackerMap[implType_], implType_, startIndex, CallDesc::makeVisitor<Method, Args...>(ARC_FWD(visitor)));
        }

        template<class Self, class Method, class... Args>
        constexpr MockReturn impl(this Self& self, Method method, Args&&... args)
        {
            auto const implType_ = implType<Method, Args...>();

            if (self.counting) [[unlikely]]
                self.recordCall(method, ARC_FWD(args)...);

            UniversalFn* impl = nullptr;
            if (auto const it = self.impls.find(implType_); it != self.impls.end())
            {
                if constexpr (std::is_const_v<Self>)
                {
                    if (it->second.con)
                        impl = &it->second.con;
                }
                else
                {
                    if (it->second.mut)
                        impl = &it->second.mut;
                    else if (it->second.con)
                        impl = &it->second.con;
                }
            }

            if (impl != nullptr)
            {
                MockReturn result;
                if constexpr (sizeof...(args) > 0)
                {
                    void* a[] = {std::addressof(args)...};
                    std::invoke(*impl, std::addressof(result), a);
                }
                else
                {
                    std::invoke(*impl, std::addressof(result), nullptr);
                }
                return result;
            }

            if (auto const retIt = self.returnsMap.find(implType_); retIt != self.returnsMap.end())
                return std::as_const(retIt->second);

            if (auto const retIt = self.returnsMap.find(methodType<Method>()); retIt != self.returnsMap.end())
                return std::as_const(retIt->second);

            MockReturn result;
            switch (self.defaultBehaviour)
            {
            case MockDefault::ReturnDefault:
                result.setReturnDefault();
                break;
            case MockDefault::ThrowIfMissing:
                throw std::runtime_error(notDefinedError<Self, Method, Args...>());
            }
            return result;
        }

        template<class... Fs>
        constexpr void define(Fs const&... fs)
        {
            (defineImpl((decltype(getTypes(&Fs::operator()))) nullptr, false, fs), ...);
            (defineImpl((decltype(getTypes(&Fs::operator()))) nullptr, true, fs), ...);
        }
        template<class... Fs>
        constexpr void defineConst(Fs&&... fs)
        {
            (defineImpl((decltype(getTypes(&std::remove_cvref_t<Fs>::operator()))) nullptr, true, ARC_FWD(fs)), ...);
        }
        template<class... Fs>
        constexpr void defineMut(Fs&&... fs)
        {
            (defineImpl((decltype(getTypes(&std::remove_cvref_t<Fs>::operator()))) nullptr, false, ARC_FWD(fs)), ...);
        }
        template<class Method, class... Args>
        constexpr void undefine()
        {
            auto const implType_ = implType<Method, Args...>();
            impls.erase(implType_);
            returnsMap.erase(implType_);
        }
        template<class Method, class... Args>
        constexpr void undefineConst()
        {
            auto const implType_ = implType<Method, Args...>();
            if (auto it = impls.find(implType_); it != impls.end())
                it->second.con.reset();
            returnsMap.erase(implType_);
        }
        template<class Method, class... Args>
        constexpr void undefineMut()
        {
            auto const implType_ = implType<Method, Args...>();
            if (auto it = impls.find(implType_); it != impls.end())
                it->second.mut.reset();
            returnsMap.erase(implType_);
        }

    private:
        struct TraitTag;
        struct MethodTag;
        struct ImplTag;

        template<IsTrait Trait>
        static TypeId traitType()
        {
            return TypeId::of<TraitTag, Trait>();
        }
        template<class Method>
        static TypeId methodType()
        {
            return TypeId::of<MethodTag, Method>();
        }
        template<class Method, class... Args>
        static TypeId implType()
        {
            return TypeId::of<ImplTag, Method, Args...>();
        }

        template<class Self, class Method, class... Args>
        ARC_COLD void recordCall(this Self& self, Method, Args&&... args)
        {
            auto const implType_ = implType<Method, Args...>();
            auto const methodType_ = methodType<Method>();
            auto const traitType_ = traitType<TraitOf<Method>>();
            auto& implTracker = self.trackerMap[implType_];
            auto& methodTracker = self.trackerMap[methodType_];
            auto& traitTracker = self.trackerMap[traitType_];

            ++self.callCount;
            ++implTracker.callCount;
            ++methodTracker.callCount;
            ++traitTracker.callCount;

            if (self.loggingAllCalls or implTracker.logIndices or methodTracker.logIndices or traitTracker.logIndices) [[unlikely]]
            {
                // Only create more specific trackers if their more general counterparts exist
                // Hierarchy: allCalls -> trait -> method -> impl
                if (self.loggingAllCalls or traitTracker.logIndices)
                {
                    if (not traitTracker.logIndices)
                        traitTracker.logIndices.emplace(self.callLog.max_size());
                    if (not methodTracker.logIndices)
                        methodTracker.logIndices.emplace(self.callLog.max_size());
                    if (not implTracker.logIndices)
                        implTracker.logIndices.emplace(self.callLog.max_size());
                }
                else if (methodTracker.logIndices and not implTracker.logIndices)
                {
                    implTracker.logIndices.emplace(self.callLog.max_size());
                }

                CallDesc::ArgVisitor argVisitor;
                if constexpr (sizeof...(args) > 0 and (std::is_copy_constructible_v<std::remove_cvref_t<Args>> and ...))
                {
                    argVisitor =
                        [...args = std::as_const(args)](void* r, UniversalFn& f)
                        {
                            void const* a[] = {std::addressof(args)...};
                            f(r, const_cast<void**>(a));
                        };
                }
                // If we will evict an entry, ensure to remove it from trackers pointing to it
                if (self.callLog.full()) [[unlikely]]
                    self.callLog.front().evictFromTrackers();
                self.callLog.emplace_back(
                    traitType_,
                    methodType_,
                    implType_,
                    std::move(argVisitor),
                    self.callLog.end_id(),
                    std::array{&implTracker, &methodTracker, &traitTracker},
                    std::is_const_v<Self>);
            }
        }

        template<class R, class F, class... Args>
        static auto getTypes(R (F::*)(Args...)) -> R(*)(Args...);
        template<class R, class F, class... Args>
        static auto getTypes(R (F::*)(Args...) const) -> R(*)(Args...);

        template<class R, class Tag, class... Args>
        constexpr void defineImpl(R(*)(Tag, Args...), bool isConst, auto&& f)
        {
            auto& defs = impls[implType<Tag, Args...>()];
            (isConst ? defs.con : defs.mut) =
                [f = ARC_FWD(f)](void* mockReturn, void** args) mutable -> void
                {
                    [&]<std::size_t... I>(std::index_sequence<I...>) -> void
                    {
                        auto& result = *static_cast<detail::MockReturn*>(mockReturn);
                        if constexpr (std::is_void_v<R>)
                        {
                            result.reset();
                            std::invoke(f, Tag{}, static_cast<Args&&>(*static_cast<std::remove_cvref_t<Args>*>(args[I]))...);
                        }
                        else
                        {
                            result.emplace<R>(
                                std::invoke(f, Tag{}, static_cast<Args&&>(*static_cast<std::remove_cvref_t<Args>*>(args[I]))...));
                        }
                    }(std::index_sequence_for<Args...>{});
                };
        }

        template<class Self, class Method, class... Args>
        static constexpr std::string notDefinedError()
        {
            std::string error = "Mock implementation not defined for impl(";
            error += arc::detail::typeName<Method>();
            ((error += ", ", error += arc::detail::typeName<Args>()), ...);
            if constexpr (std::is_const_v<Self>)
                error += ") const";
            else
                error += ')';
            return error;
        }

        struct CallTracker
        {
            std::size_t callCount = 0;
            std::optional<CircularBuffer<std::size_t>> logIndices;
        };

        struct CallDesc
        {
            template<class Optional_>
            class Visitor
            {
                friend struct CallDesc;
                using Optional = Optional_;
                explicit Visitor(TypeId implType, UniversalFn f)
                    : implType(implType)
                    , f(std::move(f))
                {}
                TypeId implType;
                UniversalFn f;
            };

            using ArgVisitor = Function<void(void* result, UniversalFn& f), FunctionPolicy{.copyable = true, .mutableCall = false, .constCall = true}>;

            explicit CallDesc(
                TypeId traitType,
                TypeId methodType,
                TypeId implType,
                ArgVisitor argVisitor,
                std::size_t callId,
                std::array<CallTracker*, 3> trackers,
                bool isConst
            )
                : traitType_(traitType)
                , methodType_(methodType)
                , implType_(implType)
                , argVisitor(std::move(argVisitor))
                , callId_(callId)
                , trackers_(trackers)
                , isConst_(isConst)
            {
                for (CallTracker* tracker : trackers_)
                    if (tracker != nullptr and tracker->logIndices)
                        tracker->logIndices->push_back(callId_);
            }

            constexpr TypeId traitType() const { return traitType_; }
            constexpr TypeId methodType() const { return methodType_; }
            constexpr TypeId implType() const { return implType_; }

            template<IsTrait Trait>
            constexpr bool isTrait() const { return traitType_ == MockBase::traitType<Trait>(); }
            template<class Method>
            constexpr bool isMethod() const { return methodType_ == MockBase::methodType<Method>(); }
            template<class Method, class... Args>
            constexpr bool isImpl() const { return methodType_ == MockBase::implType<Method, Args...>(); }

            constexpr std::size_t callIndex() const { return callId_; }
            constexpr bool isConst() const { return isConst_; }

            template<class Method, class... Args, std::invocable<std::remove_cvref_t<Args> const&...> F>
            auto visit(F&& f) const
            {
                auto visitor = makeVisitor<Method, Args...>(ARC_FWD(f));
                typename decltype(visitor)::Optional result;
                argVisitor(std::addressof(result), visitor.f);
                return result;
            }

            template<class Optional>
            auto visit(Visitor<Optional>& visitor) const
            {
                if (implType_ != visitor.implType) [[unlikely]]
                    throw std::runtime_error("CallDesc::visit: visitor does not expect the argument types of this CallDesc");
                Optional result;
                argVisitor(std::addressof(result), visitor.f);
                return result;
            }

            template<class Method, class... Args, std::invocable<std::remove_cvref_t<Args> const&...> F>
            static auto makeVisitor(F&& f)
            {
                using Result = std::invoke_result_t<F, std::remove_cvref_t<Args> const&...>;
                using Optional = std::optional<std::conditional_t<std::is_void_v<Result>, std::monostate, Result>>;

                return Visitor<Optional>(
                    MockBase::implType<Method, Args...>(),
                    [f = ARC_FWD(f)](void* r, void** args) mutable -> void
                    {
                        [&]<std::size_t... I>(std::index_sequence<I...>) -> void
                        {
                            auto& result = *static_cast<Optional*>(r);
                            if constexpr (std::is_void_v<Result>)
                            {
                                std::invoke(f, *static_cast<std::remove_cvref_t<Args> const*>(args[I])...);
                                result.emplace();
                            }
                            else
                            {
                                result.emplace(
                                    std::invoke(f, *static_cast<std::remove_cvref_t<Args> const*>(args[I])...));
                            }
                        }(std::index_sequence_for<Args...>{});
                    });
            }

        private:
            friend struct MockBase;

            void evictFromTrackers()
            {
                for (CallTracker* tracker : trackers_)
                    if (tracker != nullptr and tracker->logIndices)
                        while (not tracker->logIndices->empty() and tracker->logIndices->front() <= callId_)
                            tracker->logIndices->pop_front();
            }

            TypeId traitType_;
            TypeId methodType_;
            TypeId implType_;
            ArgVisitor argVisitor{};
            std::size_t callId_{};
            std::array<CallTracker*, 3> trackers_;
            bool isConst_{};
        };

        template<class Result_>
        struct CallLogVisitor
        {
            using Result = Result_;
            using OptionalResult = std::optional<std::conditional_t<std::is_void_v<Result>, std::monostate, Result>>;

            explicit CallLogVisitor(
                MockBase& mockBase,
                CallTracker& tracker,
                TypeId implType,
                std::size_t startIndex,
                CallDesc::Visitor<OptionalResult> visitorWrapper)
                : mockBase(mockBase)
                , tracker(tracker)
                , implType(implType)
                , startIndex(startIndex)
                , trackerIt(tracker.logIndices->cbegin())
                , trackingVersion(mockBase.trackingVersion)
                , visitorWrapper(std::move(visitorWrapper))
            {
                reset();
            }

            void validate(bool checkEvicted = false) const
            {
                if (trackingVersion != mockBase.trackingVersion) [[unlikely]]
                    throw std::runtime_error("CallLogVisitor: Mock call counting state has been invalidated, please rebind if this is expected");
                if (not mockBase.loggingAllCalls) [[unlikely]]
                    throw std::runtime_error("CallLogVisitor: Call logging is no longer enabled for this mock");
                if (checkEvicted and not trackerIt.is_valid_id()) [[unlikely]]
                    throw std::runtime_error("CallLogVisitor: Some logged calls have been evicted from the mock's call log, please reset if this is expected");
            }

            // Rebind the visitor to a new start index (useful if the mock's call log was cleared)
            void rebind(std::size_t newStartIndex = 0)
            {
                trackingVersion = mockBase.trackingVersion;
                reset(newStartIndex);
            }

            // Start reading from startIndex again
            void reset()
            {
                validate();
                std::size_t beginning = std::max(startIndex, mockBase.callLog.begin_id());
                trackerIt = std::find_if(tracker.logIndices->cbegin(), tracker.logIndices->cend(), [beginning] (std::size_t index) { return index >= beginning; });
                lastVisited.reset();
            }

            // Start reading from a different startIndex
            void reset(std::size_t newStartIndex)
            {
                startIndex = newStartIndex;
                reset();
            }

            // Visit the next matching call and advance the index. Changes lastVisitedIndex.
            OptionalResult popFront()
            {
                if (trackerIt == tracker.logIndices->cend())
                    return std::nullopt;
                validate(true);
                lastVisited = *trackerIt;
                CallDesc& callDesc = mockBase.callLog.at_id_unchecked(*lastVisited);
                ++trackerIt;
                return callDesc.visit(visitorWrapper);
            }

            // Visit the last matching call, but do not advance the front iterator. Changes lastVisitedIndex.
            OptionalResult back()
            {
                if (tracker.logIndices->empty())
                    return std::nullopt;
                lastVisited = tracker.logIndices->back();
                CallDesc& callDesc = mockBase.callLog.at_id_unchecked(*lastVisited);
                return callDesc.visit(visitorWrapper);
            }

            // Get the number of remaining matching calls from current index
            std::size_t size() const
            {
                validate();
                return std::distance(trackerIt, tracker.logIndices->cend());
            }

            bool empty() const { return size() == 0; }

            // Returns the index after the last popped
            std::optional<std::size_t> currentIndex() const
            {
                validate();
                if (trackerIt == tracker.logIndices->cend())
                    return std::nullopt;
                return *trackerIt;
            }

            // Get the index of the last visited call in the log via popFront() or back()
            std::optional<std::size_t> lastVisitedIndex() const
            {
                return lastVisited;
            }

            // Get the index of the next matching call after the current index with the given return value
            // Does not advance the index
            std::optional<std::size_t> findNext(Result const& value)
            {
                validate(true);
                std::optional<std::size_t> foundIndex;
                for (auto it = trackerIt; it != tracker.logIndices->cend(); ++it)
                {
                    decltype(auto) result = mockBase.callLog.at_id_unchecked(*it).visit(visitorWrapper);
                    if (result == value)
                    {
                        foundIndex = *it;
                        break;
                    }
                }
                return foundIndex;
            }

            // Get the index of the last matching call after the current index with the given return value
            // Does not advance the index
            std::optional<std::size_t> findLast(Result const& value) const
            {
                validate();
                std::optional<std::size_t> foundIndex;
                for (auto it = tracker.logIndices->cend() - 1; it > trackerIt; --it)
                {
                    decltype(auto) result = mockBase.callLog.at_id_unchecked(*it).visit(visitorWrapper);
                    if (result == value)
                    {
                        foundIndex = *it;
                        break;
                    }
                }
                return foundIndex;
            }

        private:
            MockBase& mockBase;
            CallTracker& tracker;
            TypeId implType;
            std::size_t startIndex;
            CircularBuffer<std::size_t>::const_iterator trackerIt;
            std::size_t trackingVersion;
            std::optional<std::size_t> lastVisited;
            CallDesc::Visitor<OptionalResult> visitorWrapper;
        };

        MockDefault defaultBehaviour{};
        bool counting{};
        bool loggingAllCalls{};
        std::size_t trackingVersion{0};
        mutable std::size_t callCount{0};
        mutable std::unordered_map<TypeId, MockDefs> impls;
        mutable std::unordered_map<TypeId, MockReturn> returnsMap;
        mutable std::unordered_map<TypeId, CallTracker> trackerMap;
        mutable CircularBuffer<CallDesc> callLog;
    };

} // namespace detail

ARC_MODULE_EXPORT
template<class DefaultTypes/* = EmptyTypes*/, class... MockedTraits>
struct Mock : detail::MockBase
{
    using Traits = std::conditional_t<
        sizeof...(MockedTraits) == 0,
        arc::TraitsOpen<Mock>,
        arc::Traits<Mock, MockedTraits...>
    >;

    using detail::MockBase::MockBase;

    using Types = DefaultTypes;
};

}

#endif // INCLUDE_ARC_MOCK_HPP
