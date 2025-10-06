#ifndef INCLUDE_ARC_DETAIL_CAST_HPP
#define INCLUDE_ARC_DETAIL_CAST_HPP

#include "arc/compiler.hpp"
#include "arc/macros.hpp"

#if !ARC_IMPORT_STD
#include <bit>
#include <cstdint>
#include <memory>
#include <type_traits>
#endif

// Define the macro here (not in macros.hpp) so that it is not accessible for module users.
#define ARC_MEM_PTR(Class, member) \
    ::arc::detail::memberPtr<Class>(&Class::member)

namespace arc::detail {

template<class Derived, class Base>
ARC_INLINE constexpr Derived& downCast(Base& base)
{
    static_assert(std::is_base_of_v<Base, Derived>);
    if constexpr (not isMsvc) // MSVC doesn't have proper EBO
        static_assert(sizeof(Derived) == sizeof(Base));
    return *(Derived*)std::addressof(base);
}

template<class Derived, class Base>
ARC_INLINE constexpr Derived const& downCast(Base const& base)
{
    static_assert(std::is_base_of_v<Base, Derived>);
    if constexpr (not isMsvc) // MSVC doesn't have proper EBO
        static_assert(sizeof(Derived) == sizeof(Base));
    return *(Derived const*)std::addressof(base);
}

template<class Base, class Derived>
ARC_INLINE constexpr Base& upCast(Derived& derived)
{
    static_assert(std::is_base_of_v<Base, Derived>);
    return *(Base*)std::addressof(derived);
}

template<class Base, class Derived>
ARC_INLINE constexpr Base const& upCast(Derived const& derived)
{
    static_assert(std::is_base_of_v<Base, Derived>);
    return *(Base const*)std::addressof(derived);
}


#if __cpp_lib_is_virtual_base_of >= 202406L
template<class Base, class Derived>
concept IsVirtualBaseOf = std::is_virtual_base_of_v<Base, Derived>;
#else
template<class Base, class Derived>
concept IsVirtualBaseOf = std::is_base_of_v<Base, Derived>
    and requires(Base* base, Derived* derived)
    {
        // Check if Derived* can be converted to Base*
        (Base*)derived;
        // Assert that Base* cannot be converted to Derived*
        // A pointer to a virtual base class cannot be downcast to the derived class pointer
        requires not requires { (Derived*)base; };
    };
#endif

// MemberPtr stores a pointer to data member guaranteed to be represented as an offset in all known ABIs.
template<class Class, class Member>
struct MemberPtr
{
    ARC_INLINE auto& getClassFromMember(auto& member) const
    {
        return downCast<Member>(member).*invert();
    }

    ARC_INLINE auto& getMemberFromClass(auto& classInstance) const
    {
        return classInstance.*memPtr;
    }

    template<class InnerMember>
    ARC_INLINE MemberPtr<Class, InnerMember> operator+(MemberPtr<Member, InnerMember> inner) const
    {
        return fromOffset<Class, InnerMember>(toOffset() + inner.toOffset());
    }

    template<class OuterClass>
    ARC_INLINE MemberPtr<OuterClass, Member> operator+(MemberPtr<OuterClass, Class> outer) const
    {
        return fromOffset<OuterClass, Member>(outer.toOffset() + toOffset());
    }

    ARC_INLINE auto toOffset() const
    {
        // Not constexpr, but well defined (ABI implementation defined)
        // Safe to assume that the member pointer is represented as an offset
        // since we guarantee it does not point to a member of a virtual base class.
        // All supported ABIs represent such a pointer-to-data-member as an offset.
        return std::bit_cast<IntFor<decltype(memPtr)>>(memPtr);
    }

    template<class BaseClass>
    constexpr MemberPtr(Member BaseClass::* memPtr) : memPtr(memPtr)
    {
        static_assert(std::is_base_of_v<BaseClass, Class>);
        static_assert(not IsVirtualBaseOf<BaseClass, Class>);
        if consteval
        {
            if (memPtr == nullptr)
                throw "nullptr not allowed";
        }
    }

private:
    template<class P>
    using IntFor = std::conditional_t<sizeof(P) == 8, std::int64_t, std::int32_t>;

    template<class C, class M>
    friend struct MemberPtr;

    Member Class::* memPtr;

    ARC_INLINE auto invert() const
    {
        using Int = IntFor<Class Member::*>;
        return std::bit_cast<Class Member::*>(-static_cast<Int>(toOffset()));
    }

    template<class C, class M>
    ARC_INLINE static MemberPtr<C, M> fromOffset(std::integral auto offset)
    {

        // Not constexpr, but well defined (ABI implementation defined)
        // Safe to assume that the member pointer is represented as an offset
        // since we guarantee it does not point to a member of a virtual base class.
        // All supported ABIs represent such a pointer-to-data-member as an offset.
        return std::bit_cast<M C::*>(static_cast<IntFor<M C::*>>(offset));
    }
};

ARC_MODULE_EXPORT
template<class Class, class BaseClass, class Member>
ARC_INLINE constexpr MemberPtr<Class, Member> memberPtr(Member BaseClass::* memPtr)
{
    return MemberPtr<Class, Member>(memPtr);
}

} // namespace arc::detail


#endif // INCLUDE_ARC_DETAIL_CAST_HPP
