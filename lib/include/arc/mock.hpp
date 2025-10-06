#ifndef INCLUDE_ARC_MOCK_HPP
#define INCLUDE_ARC_MOCK_HPP

#include "arc/context_fwd.hpp"
#include "arc/detail/type_name.hpp"
#include "arc/empty_types.hpp"
#include "arc/macros.hpp"
#include "arc/mock_fwd.hpp"
#include "arc/node.hpp"
#include "arc/test_context.hpp"
#include "arc/test.hpp"
#include "arc/trait.hpp"
#include "arc/traits.hpp"

#if !ARC_IMPORT_STD
#include <any>
#include <functional>
#include <map>
#include <span>
#include <string>
#include <string_view>
#include <stdexcept>
#include <typeindex>
#include <typeinfo>
#include <type_traits>
#include <variant>
#include <vector>
#endif

namespace arc::test {

namespace detail {

    struct MockReturn
    {
        template<class T>
        constexpr operator T&() const &&
        {
            if (Ref const* ref = std::get_if<Ref>(&value))
            {
                if (ref->type == std::type_index{typeid(T&)} and ref->lref)
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
                throw std::bad_any_cast();
            }
            else if (Ref* ref = std::get_if<Ref>(&value))
            {
                if (ref->type != std::type_index{typeid(T&)} or not ref->lref)
                    throw std::bad_any_cast();
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
                    return std::move(*p);
            }
            else if (Ref* ref = std::get_if<Ref>(&value))
            {
                if (ref->type == std::type_index{typeid(T&&)} and not ref->lref)
                    return std::move(*static_cast<T*>(ref->ptr));
            }
            else if (returnDefault)
            {
                return std::move(value.emplace<std::any>().emplace<T>());
            }
            throw std::bad_any_cast();
        }

        template<class T>
        constexpr void emplace(auto&& arg)
        {
            if constexpr (std::is_reference_v<T>)
                value.emplace<Ref>(std::addressof(arg), typeid(T), std::is_lvalue_reference_v<T>);
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
            std::type_index type;
            bool lref = false;
        };
        std::variant<std::monostate, std::any, Ref> value;
    };

    using MockDef = std::function<void(MockReturn& result, void** args)>;
    struct MockDefs
    {
        MockDef con, mut;
    };

    enum MockDefaultBehaviour
    {
        ReturnDefault,
        ThrowIfMissing,
    };

} // namespace detail

ARC_MODULE_EXPORT
template<class DefaultTypes/* = EmptyTypes*/, class... MockedTraits>
struct Mock
{
    using ArgTypes = std::vector<std::type_index>;

    template<class Context>
    struct Node : arc::test::TestOnlyNode
    {
        using Traits = std::conditional_t<
            sizeof...(MockedTraits) == 0,
            arc::TraitsOpen<Node>,
            arc::Traits<Node, MockedTraits...>
        >;

        Node() = default;

        using Types = DefaultTypes;

        constexpr void reset()
        {
            *this = {};
        }

        constexpr void setReturnDefault() { defaultBehaviour = detail::MockDefaultBehaviour::ReturnDefault; }
        constexpr void setThrowIfMissing() { defaultBehaviour = detail::MockDefaultBehaviour::ThrowIfMissing; }

        template<class Tag>
        constexpr std::size_t methodCallCount(Tag) const
        {
            auto const it = methodCountMap.find(std::type_index{typeid(Tag)});
            return it != methodCountMap.end() ? it->second : 0ul;
        }
        template<IsTrait Tag>
        constexpr std::size_t traitCallCount(Tag) const
        {
            auto const it = traitCountMap.find(std::type_index{typeid(Tag)});
            return it != traitCountMap.end() ? it->second : 0ul;
        }
        template<class Tag, class... Args>
        constexpr std::size_t definitionCallCount() const
        {
            ArgTypes argTypes{
                std::type_index{typeid(Tag)},
                std::type_index{typeid(Args)}...};

            auto const it = definitionCountMap.find(argTypes);
            return it != definitionCountMap.end() ? it->second : 0ul;
        }

        template<class Self, class Method, class... Args>
        constexpr detail::MockReturn impl(this Self& self, Method, Args&&... args)
        {
            ArgTypes argTypes{
                std::type_index{typeid(Method)},
                std::type_index{typeid(Args)}...};

            self.methodCountMap[std::type_index{typeid(Method)}]++;
            self.traitCountMap[std::type_index{typeid(arc::TraitOf<Method>)}]++;
            self.definitionCountMap[argTypes]++;

            detail::MockDef* impl = nullptr;
            if (auto const it = self.definitions.find(argTypes); it != self.definitions.end())
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
                case detail::MockDefaultBehaviour::ReturnDefault:
                    result.setReturnDefault();
                    break;
                case detail::MockDefaultBehaviour::ThrowIfMissing:
                    throw std::runtime_error(notDefinedError<Self, Method, Args...>());
                }
            }

            return result;
        }

        template<class... Fs>
        constexpr void define(Fs... fs)
        {
            (defineImpl((decltype(getTypes(&Fs::operator()))) nullptr, false, fs), ...);
            (defineImpl((decltype(getTypes(&Fs::operator()))) nullptr, true, fs), ...);
        }
        template<class... Fs>
        constexpr void defineConst(Fs... fs)
        {
            (defineImpl((decltype(getTypes(&Fs::operator()))) nullptr, true, std::move(fs)), ...);
        }
        template<class... Fs>
        constexpr void defineMut(Fs... fs)
        {
            (defineImpl((decltype(getTypes(&Fs::operator()))) nullptr, false, std::move(fs)), ...);
        }

    private:
        template<class R, class F, class... Args>
        static auto getTypes(R (F::*)(Args...)) -> R(*)(Args...);
        template<class R, class F, class... Args>
        static auto getTypes(R (F::*)(Args...) const) -> R(*)(Args...);

        template<class R, class Tag, class... Args>
        constexpr void defineImpl(R(*)(Tag, Args...), bool isConst, auto&& f)
        {
            ArgTypes argTypes{
                std::type_index{typeid(std::remove_cvref_t<Tag>)},
                std::type_index{typeid(Args)}...};
            auto& defs = definitions[argTypes];
            (isConst ? defs.con : defs.mut) =
                [f = ARC_FWD(f)](ARC_IF_NOT_MSVC(this auto&,) detail::MockReturn& result, void** args) -> void
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

        detail::MockDefaultBehaviour defaultBehaviour = detail::MockDefaultBehaviour::ReturnDefault;
        mutable std::map<ArgTypes, detail::MockDefs> definitions;
        mutable std::map<ArgTypes, std::size_t> definitionCountMap;
        mutable std::map<std::type_index, std::size_t> traitCountMap;
        mutable std::map<std::type_index, std::size_t> methodCountMap;
    };
};

}

#endif // INCLUDE_ARC_MOCK_HPP
