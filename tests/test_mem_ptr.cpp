#include <doctest/doctest.h>

#include "arc/detail/cast.hpp"
#include "arc/macros.hpp"

namespace arc::tests {

struct A { int ai; };
struct B { int bi; };
struct C { int ci; };
struct ABc : A, B { C c; };
struct BCa : B, C { A a; };
struct AvB : A, virtual B { C c; virtual ~AvB() = default; AvB() = default; };
struct vBC : virtual B, C { A a; };

struct Test0
{
    A a;
    B b;
    C c;
    int x;
};

struct Test1 : A, B, C
{
    int x;
};

struct Test2 : ABc, BCa
{
    int x;
};

struct Test3 : AvB, vBC
{
    int x;
};


TEST_CASE("Member Pointer")
{
    Test0 t0{.a{1}, .b{2}, .c{3}, .x = 4};
    Test1 t1{{5}, {6}, {7}, 8};
    Test2 t2{{{9}, {10}, {11}}, {{12}, {13}, {14}}, 15};
    Test3 t3;
    t3.ai = 16;
    t3.bi = 17;
    t3.ci = 18;
    t3.c.ci = 19;
    t3.a.ai = 20;
    t3.x = 21;

    CHECK(ARC_MEM_PTR(Test0, a).getMemberFromClass(t0).ai == 1);
    CHECK(ARC_MEM_PTR(Test0, b).getMemberFromClass(t0).bi == 2);
    CHECK(ARC_MEM_PTR(Test0, c).getMemberFromClass(t0).ci == 3);
    CHECK(ARC_MEM_PTR(Test0, x).getMemberFromClass(t0) == 4);
    CHECK(ARC_MEM_PTR(Test1, ai).getMemberFromClass(t1) == 5);
    CHECK(ARC_MEM_PTR(Test1, bi).getMemberFromClass(t1) == 6);
    CHECK(ARC_MEM_PTR(Test1, ci).getMemberFromClass(t1) == 7);
    CHECK(ARC_MEM_PTR(Test1, x).getMemberFromClass(t1) == 8);
    CHECK(ARC_MEM_PTR(Test2, a).getMemberFromClass(t2).ai == 14);
    // B is an ambiguous member of Test2, so this correctly fails to compile
    // CHECK(ARC_MEM_PTR(Test2, b).getMemberFromClass(t2) == 10);
    CHECK(ARC_MEM_PTR(Test2, c).getMemberFromClass(t2).ci == 11);
    CHECK(ARC_MEM_PTR(Test2, x).getMemberFromClass(t2) == 15);
    CHECK(ARC_MEM_PTR(Test3, a).getMemberFromClass(t3).ai == 20);
    // B is a virtual member of Test3, so this correctly fails to compile
    // CHECK(ARC_MEM_PTR(Test3, bi).getMemberFromClass(t3) == 15);
    CHECK(ARC_MEM_PTR(Test3, c).getMemberFromClass(t3).ci == 19);
    CHECK(ARC_MEM_PTR(Test3, x).getMemberFromClass(t3) == 21);

    CHECK(&(ARC_MEM_PTR(Test0, a).getClassFromMember(t0.a)) == &t0);
    CHECK(&(ARC_MEM_PTR(Test0, b).getClassFromMember(t0.b)) == &t0);
    CHECK(&(ARC_MEM_PTR(Test0, c).getClassFromMember(t0.c)) == &t0);
    CHECK(&(ARC_MEM_PTR(Test2, a).getClassFromMember(t2.a)) == &t2);
    CHECK(&(ARC_MEM_PTR(Test2, c).getClassFromMember(t2.c)) == &t2);
    CHECK(&(ARC_MEM_PTR(Test3, a).getClassFromMember(t3.a)) == &t3);
    CHECK(&(ARC_MEM_PTR(Test3, c).getClassFromMember(t3.c)) == &t3);
}

struct Empty {};
struct Int { int value; };
struct Char { char value; };
struct Aligned { alignas(16) Int a; Char b; };
#if ARC_COMPILER_MSVC
#pragma pack(push, 1)
struct Packed { Int a; Char b; };
#pragma pack(pop)
#else
struct [[gnu::packed]] Packed { Int a; Char b; };
#endif

struct ConstVolatile {
    Int const ci;
    Int volatile vi;
    Int arr[3];
    Int* pi;
    ConstVolatile(int v) : ci{v}, vi{v+1}, arr{{v},{v+2},{v+3}}, pi(&arr[0]) {}
};

struct ZeroOffset { Int first; Int second; };

struct WithEmptyBase : Empty { Int x; };

TEST_CASE("MemberPtr edge cases")
{
    ConstVolatile cv(42);
    CHECK(ARC_MEM_PTR(ConstVolatile, ci).getMemberFromClass(cv).value == 42);
    CHECK(ARC_MEM_PTR(ConstVolatile, vi).getMemberFromClass(cv).value == 43);
    CHECK(ARC_MEM_PTR(ConstVolatile, arr).getMemberFromClass(cv)[0].value == 42);
    CHECK(ARC_MEM_PTR(ConstVolatile, arr).getMemberFromClass(cv)[2].value == 45);
    CHECK(ARC_MEM_PTR(ConstVolatile, pi).getMemberFromClass(cv)->value == 42);

    CHECK((&(ARC_MEM_PTR(ConstVolatile, ci).getClassFromMember(cv.ci)) == &cv));
    CHECK((&(ARC_MEM_PTR(ConstVolatile, vi).getClassFromMember(cv.vi)) == &cv));

    ZeroOffset zo{{100}, {200}};
    CHECK(ARC_MEM_PTR(ZeroOffset, first).getMemberFromClass(zo).value == 100);
    CHECK(ARC_MEM_PTR(ZeroOffset, second).getMemberFromClass(zo).value == 200);
    CHECK(&(ARC_MEM_PTR(ZeroOffset, first).getClassFromMember(zo.first)) == &zo);
    CHECK(&(ARC_MEM_PTR(ZeroOffset, second).getClassFromMember(zo.second)) == &zo);

    WithEmptyBase web; web.x.value = 77;
    CHECK(ARC_MEM_PTR(WithEmptyBase, x).getMemberFromClass(web).value == 77);

    CHECK(&(ARC_MEM_PTR(WithEmptyBase, x).getClassFromMember(web.x)) == &web);

    Aligned al; al.a.value = 123; al.b.value = 'z';
    CHECK(ARC_MEM_PTR(Aligned, a).getMemberFromClass(al).value == 123);
    CHECK(ARC_MEM_PTR(Aligned, b).getMemberFromClass(al).value == 'z');

    CHECK(&(ARC_MEM_PTR(Aligned, a).getClassFromMember(al.a)) == &al);
    CHECK(&(ARC_MEM_PTR(Aligned, b).getClassFromMember(al.b)) == &al);

    Packed pk; pk.a.value = 321; pk.b.value = 'y';
    CHECK(ARC_MEM_PTR(Packed, a).getMemberFromClass(pk).value == 321);
    CHECK(ARC_MEM_PTR(Packed, b).getMemberFromClass(pk).value == 'y');

#if !ARC_COMPILER_GCC // GCC won't compile this
    CHECK(&(ARC_MEM_PTR(Packed, a).getClassFromMember(pk.a)) == &pk);
#endif
    CHECK(&(ARC_MEM_PTR(Packed, b).getClassFromMember(pk.b)) == &pk);
}

}
