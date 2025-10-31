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
    bool tracking = false;

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

    using MockDef = arc::Function<void(MockReturn& result, void** args), arc::FunctionPolicy{.copyable = true, .mutableCall = true, .constCall = false}>;
    struct MockDefs
    {
        MockDef con, mut;
    };

    struct MockBase : arc::test::TestOnlyNode
    {
        constexpr void setReturnDefault() { defaultBehaviour = MockDefault::ReturnDefault; }
        constexpr void setThrowIfMissing() { defaultBehaviour = MockDefault::ThrowIfMissing; }
        constexpr bool returnsDefault() const { return defaultBehaviour == MockDefault::ReturnDefault; }
        constexpr bool throwsIfMissing() const { return defaultBehaviour == MockDefault::ThrowIfMissing; }

        constexpr void enableCallTracking() { tracking = true; }
        constexpr void disableCallTracking() { tracking = false; }
        constexpr bool callTrackingEnabled() const { return tracking; }

        void resetTracking()
        {
            definitionCountMap.clear();
            traitCountMap.clear();
            methodCountMap.clear();
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
            tracking = params.tracking;
        }

        template<class Tag>
        std::size_t methodCallCount(Tag) const
        {
            if (not tracking)
                throw std::runtime_error("methodCallCount: Call tracking is not enabled for this mock");
            auto const it = methodCountMap.find(TypeId::of<Tag>());
            return it != methodCountMap.end() ? it->second : 0ul;
        }
        template<IsTrait Tag>
        std::size_t traitCallCount(Tag) const
        {
            if (not tracking)
                throw std::runtime_error("traitCallCount: Call tracking is not enabled for this mock");
            auto const it = traitCountMap.find(TypeId::of<Tag>());
            return it != traitCountMap.end() ? it->second : 0ul;
        }
        template<class Tag, class... Args>
        std::size_t definitionCallCount() const
        {
            if (not tracking)
                throw std::runtime_error("definitionCallCount: Call tracking is not enabled for this mock");
            auto const it = definitionCountMap.find(argTypes<Tag, Args...>());
            return it != definitionCountMap.end() ? it->second : 0ul;
        }

        template<class Self, class Method, class... Args>
        constexpr detail::MockReturn impl(this Self& self, Method, Args&&... args)
        {
            auto const argTypes_ = argTypes<Method, Args...>();

            if (self.tracking) [[unlikely]]
            {
                self.methodCountMap[TypeId::of<Method>()]++;
                self.traitCountMap[TypeId::of<TraitOf<Method>>()]++;
                self.definitionCountMap[argTypes_]++;
            }

            detail::MockDef* impl = nullptr;
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

            detail::MockReturn result;
            if (impl != nullptr)
            {
                if constexpr (sizeof...(args) > 0)
                {
                    void* a[] = {std::addressof(args)...};
                    std::invoke(*impl, result, a);
                }
                else
                {
                    std::invoke(*impl, result, nullptr);
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

    protected:
        constexpr explicit MockBase(MockParams params = {})
            : defaultBehaviour(params.defaultBehaviour)
            , tracking(params.tracking)
        {}

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
                [f = ARC_FWD(f)](detail::MockReturn& result, void** args) mutable -> void
                {
                    [&]<std::size_t... I>(std::index_sequence<I...>) -> void
                    {
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

        MockDefault defaultBehaviour{};
        bool tracking{};
        mutable std::unordered_map<TypeId, detail::MockDefs> definitions;
        mutable std::unordered_map<TypeId, std::size_t> definitionCountMap;
        mutable std::unordered_map<TypeId, std::size_t> traitCountMap;
        mutable std::unordered_map<TypeId, std::size_t> methodCountMap;
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

    using Types = DefaultTypes;
};

}

#endif // INCLUDE_ARC_MOCK_HPP
