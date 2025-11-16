#ifndef INCLUDE_ARC_MOCK_HPP
#define INCLUDE_ARC_MOCK_HPP

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
#include <vector>
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
    bool logCalls = false;

    auto operator<=>(MockParams const&) const = default;
};

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
            , logCalls(params.logCalls)
        {}

        constexpr void setReturnDefault() { defaultBehaviour = MockDefault::ReturnDefault; }
        constexpr void setThrowIfMissing() { defaultBehaviour = MockDefault::ThrowIfMissing; }
        constexpr bool returnsDefault() const { return defaultBehaviour == MockDefault::ReturnDefault; }
        constexpr bool throwsIfMissing() const { return defaultBehaviour == MockDefault::ThrowIfMissing; }

        constexpr void enableCallCounting(bool logCalls = true)
        {
            counting = true;
            this->logCalls = logCalls;
        }
        constexpr void disableCallCounting()
        {
            counting = false;
            logCalls = false;
        }
        constexpr void enableCallLogging()
        {
            counting = true;
            logCalls = true;
        }
        constexpr void disableCallLogging() { logCalls = false; }
        constexpr bool callCountingEnabled() const { return counting; }
        constexpr bool callLoggingEnabled() const { return logCalls; }

        void resetTracking()
        {
            callCount = 0;
            definitionCountMap.clear();
            traitCountMap.clear();
            methodCountMap.clear();
            callLog.clear();
            ++trackingVersion;
        }

        void resetDefinitions()
        {
            definitions.clear();
        }

        void resetTrackingAndDefinitions()
        {
            resetTracking();
            resetDefinitions();
        }

        void reinitialise(MockParams params = {})
        {
            resetTrackingAndDefinitions();
            defaultBehaviour = params.defaultBehaviour;
            counting = params.counting;
            logCalls = params.logCalls;
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
            auto const it = traitCountMap.find(TypeId::of<Trait>());
            return it != traitCountMap.end() ? it->second : 0ul;
        }
        template<class Method>
        std::size_t methodCallCount(Method = {}) const
        {
            if (not counting)
                throw std::runtime_error("methodCallCount: Call counting is not enabled for this mock");
            auto const it = methodCountMap.find(TypeId::of<Method>());
            return it != methodCountMap.end() ? it->second : 0ul;
        }
        template<class Method, class... Args>
        std::size_t implCallCount() const
        {
            if (not counting)
                throw std::runtime_error("implCallCount: Call counting is not enabled for this mock");
            auto const it = definitionCountMap.find(argTypes<Method, Args...>());
            return it != definitionCountMap.end() ? it->second : 0ul;
        }

        template<class Method, class... Args, std::invocable<std::remove_cvref_t<Args> const&...> F>
        auto visitCalls(F&& visitor)
        {
            return visitCalls<Method, Args...>(0, ARC_FWD(visitor));
        }

        template<class Method, class... Args, std::invocable<std::remove_cvref_t<Args> const&...> F>
        auto visitCalls(std::size_t startIndex, F&& visitor)
        {
            static_assert(sizeof...(Args) > 0, "Cannot visit last call with no arguments");
            if (not logCalls)
                throw std::runtime_error("visitCalls: Call logging is not enabled for this mock");

            using result_t = std::invoke_result_t<F, std::remove_cvref_t<Args> const&...>;
            using optional_t = std::optional<std::conditional_t<std::is_void_v<result_t>, std::monostate, result_t>>;

            UniversalFn visitorWrapper(
                [visitor](void* r, void** args) mutable -> void
                {
                    [&]<std::size_t... I>(std::index_sequence<I...>) -> void
                    {
                        auto& result = *static_cast<optional_t*>(r);
                        if constexpr (std::is_void_v<result_t>)
                        {
                            std::invoke(visitor, *static_cast<std::remove_cvref_t<Args> const*>(args[I])...);
                            result.emplace();
                        }
                        else
                        {
                            result.emplace(
                                std::invoke(visitor, *static_cast<std::remove_cvref_t<Args> const*>(args[I])...));
                        }
                    }(std::index_sequence_for<Args...>{});
                });

            return CallVisitor<result_t>(*this, argTypes<Method, Args...>(), startIndex, std::move(visitorWrapper));
        }

        template<class Self, class Method, class... Args>
        constexpr MockReturn impl(this Self& self, Method, Args&&... args)
        {
            auto const argTypes_ = argTypes<Method, Args...>();

            if (self.counting) [[unlikely]]
            {
                auto const traitType = TypeId::of<TraitOf<Method>>();
                auto const methodType = TypeId::of<Method>();
                self.callCount++;
                self.traitCountMap[traitType]++;
                self.methodCountMap[methodType]++;
                self.definitionCountMap[argTypes_]++;
                if (self.logCalls) [[unlikely]]
                {
                    auto& callDesc = self.callLog.emplace_back(
                        CallDesc{
                            .traitType = traitType,
                            .methodType = methodType,
                            .argTypes = argTypes_,
                        });
                    if constexpr (sizeof...(args) > 0)
                    {
                        callDesc.visitor =
                            [args...](void* r, UniversalFn& f)
                            {
                                void const* a[] = {std::addressof(args)...};
                                f(r, const_cast<void**>(a));
                            };
                    }
                }
            }

            UniversalFn* impl = nullptr;
            if (auto const it = self.definitions.find(argTypes_); it != self.definitions.end())
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

            MockReturn result;
            if (impl != nullptr)
            {
                if constexpr (sizeof...(args) > 0)
                {
                    void* a[] = {std::addressof(args)...};
                    std::invoke(*impl, std::addressof(result), a);
                }
                else
                {
                    std::invoke(*impl, std::addressof(result), nullptr);
                }
            }
            else
            {
                switch (self.defaultBehaviour)
                {
                case MockDefault::ReturnDefault:
                    result.setReturnDefault();
                    break;
                case MockDefault::ThrowIfMissing:
                    throw std::runtime_error(notDefinedError<Self, Method, Args...>());
                }
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

    private:
        template<class Method, class... Args>
        static constexpr TypeId argTypes()
        {
            return TypeId::of<std::remove_cvref_t<Method>, Args...>();
        }

        template<class R, class F, class... Args>
        static auto getTypes(R (F::*)(Args...)) -> R(*)(Args...);
        template<class R, class F, class... Args>
        static auto getTypes(R (F::*)(Args...) const) -> R(*)(Args...);

        template<class R, class Tag, class... Args>
        constexpr void defineImpl(R(*)(Tag, Args...), bool isConst, auto&& f)
        {
            auto& defs = definitions[argTypes<Tag, Args...>()];
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

        struct CallDesc
        {
            TypeId traitType;
            TypeId methodType;
            TypeId argTypes;
            Function<void(void* result, UniversalFn& f), FunctionPolicy{.copyable = true, .mutableCall = false, .constCall = true}> visitor{};
        };

        template<class Result_>
        struct CallVisitor
        {
            using Result = Result_;
            using OptionalResult = std::optional<std::conditional_t<std::is_void_v<Result>, std::monostate, Result>>;

            explicit CallVisitor(
                MockBase& mockBase,
                TypeId argTypes,
                std::size_t startIndex,
                UniversalFn visitorWrapper)
                : mockBase(mockBase)
                , argTypes(argTypes)
                , startIndex(startIndex)
                , index(startIndex)
                , trackingVersion(mockBase.trackingVersion)
                , visitorWrapper(std::move(visitorWrapper))
            {}

            void validate() const
            {
                if (trackingVersion != mockBase.trackingVersion) [[unlikely]]
                    throw std::runtime_error("CallVisitor: Mock call counting state has been invalidated, please rebind if this is expected");
                if (not mockBase.logCalls) [[unlikely]]
                    throw std::runtime_error("CallVisitor: Call logging is no longer enabled for this mock");
            }

            // Rebind the visitor to a new start index (useful if the mock's call log was cleared)
            void rebind(std::size_t newStartIndex = 0)
            {
                if (newStartIndex > mockBase.callLog.size()) [[unlikely]]
                    throw std::out_of_range("CallVisitor::rebind: newStartIndex is out of range");

                startIndex = newStartIndex;
                index = startIndex;
                lastVisited.reset();
                trackingVersion = mockBase.trackingVersion;
            }

            // Start reading from startIndex again
            void reset()
            {
                validate();
                index = startIndex;
                lastVisited.reset();
            }

            // Start reading from a different startIndex
            void reset(std::size_t newStartIndex)
            {
                validate();
                if (newStartIndex > mockBase.callLog.size()) [[unlikely]]
                    throw std::out_of_range("CallVisitor::rebind: newStartIndex is out of range");

                index = startIndex = newStartIndex;
                lastVisited.reset();
            }

            // Visit the next matching call and advance the index. Changes lastVisitedIndex.
            OptionalResult popFront()
            {
                validate();
                OptionalResult result;
                for (auto it = mockBase.callLog.begin() + index; it != mockBase.callLog.end(); ++it)
                {
                    ++index;
                    if (it->argTypes == argTypes)
                    {
                        it->visitor(std::addressof(result), visitorWrapper);
                        lastVisited = std::distance(mockBase.callLog.begin(), it);
                        break;
                    }
                }
                return result;
            }

            // Visit the last matching call, but do not advance the index. Changes lastVisitedIndex.
            OptionalResult back()
            {
                validate();
                OptionalResult result;
                for (auto it = mockBase.callLog.rbegin(); it != mockBase.callLog.rend(); ++it)
                {
                    if (it->argTypes == argTypes)
                    {
                        it->visitor(std::addressof(result), visitorWrapper);
                        lastVisited = std::distance(it, mockBase.callLog.rend()) - 1;
                        break;
                    }
                }
                return result;
            }

            // Get the number of remaining matching calls from current index
            std::size_t size() const
            {
                validate();
                std::size_t count = 0;
                for (auto it = mockBase.callLog.begin() + index; it != mockBase.callLog.end(); ++it)
                    if (it->argTypes == argTypes)
                        ++count;
                return count;
            }

            bool empty() const { return 0 == size(); }

            // Returns the index after the last popped
            std::size_t currentIndex() const
            {
                validate();
                return index;
            }

            // Does not advance the index
            std::optional<std::size_t> nextMatchingIndex() const
            {
                validate();
                for (auto it = mockBase.callLog.begin() + index; it != mockBase.callLog.end(); ++it)
                    if (it->argTypes == argTypes)
                        return std::distance(mockBase.callLog.begin(), it);
                return std::nullopt;
            }

            // Get the index of the last visited call in the log via popFront() or back()
            std::optional<std::size_t> lastVisitedIndex() const
            {
                validate();
                return lastVisited;
            }

            // Get the index of the next matching call after the current index with the given return value
            // Does not advance the index
            std::optional<std::size_t> findNext(Result const& value)
            {
                validate();
                std::optional<std::size_t> foundIndex;
                for (auto it = mockBase.callLog.begin() + index; it != mockBase.callLog.end(); ++it)
                {
                    if (it->argTypes == argTypes)
                    {
                        OptionalResult result;
                        it->visitor(std::addressof(result), visitorWrapper);
                        if (result == value)
                        {
                            foundIndex = std::distance(mockBase.callLog.begin(), it);
                            break;
                        }
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
                for (auto it = mockBase.callLog.rbegin(); it != mockBase.callLog.rend() - index; ++it)
                {
                    if (it->argTypes == argTypes)
                    {
                        OptionalResult result;
                        it->visitor(std::addressof(result), visitorWrapper);
                        if (result == value)
                        {
                            foundIndex = std::distance(it, mockBase.callLog.rbegin()) - 1;
                            break;
                        }
                    }
                }
                return foundIndex;
            }

        private:
            MockBase& mockBase;
            TypeId argTypes;
            std::size_t startIndex;
            std::size_t index;
            std::size_t trackingVersion;
            std::optional<std::size_t> lastVisited;
            UniversalFn visitorWrapper;
        };

        MockDefault defaultBehaviour{};
        bool counting{};
        bool logCalls{};
        std::size_t trackingVersion{0};
        mutable std::size_t callCount{0};
        mutable std::unordered_map<TypeId, MockDefs> definitions;
        mutable std::unordered_map<TypeId, std::size_t> definitionCountMap;
        mutable std::unordered_map<TypeId, std::size_t> traitCountMap;
        mutable std::unordered_map<TypeId, std::size_t> methodCountMap;
        mutable std::vector<CallDesc> callLog;
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
