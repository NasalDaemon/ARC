#include <doctest/doctest.h>
#include "arc/macros.hpp"

#if !ARC_IMPORT_STD
#include <array>
#include <limits>
#include <vector>
#include <stdexcept>
#endif

import arc;

namespace arc::tests::circular_buffer {

TEST_CASE("CircularBuffer construction") {
    SUBCASE("default construction") {
        CircularBuffer<int> cb;
        CHECK(cb.empty());
        CHECK(cb.size() == 0);
        CHECK(cb.max_size() == 1024);
        CHECK(cb.capacity() == 0);
    }

    SUBCASE("construction with max size") {
        CircularBuffer<int> cb(16);
        CHECK(cb.empty());
        CHECK(cb.size() == 0);
        CHECK(cb.max_size() == 16);
        CHECK(cb.capacity() == 0);
    }

    SUBCASE("invalid max size") {
        CHECK_THROWS_AS(CircularBuffer<int>(0), std::invalid_argument);
    }

    auto defaultMaxSize = CircularBuffer<int>::defaultMaxSize;
    SUBCASE("construction with initializer list") {
        CircularBuffer<int> cb{1, 2, 3, 4};
        CHECK(cb.size() == 4);
        CHECK(cb.front() == 1);
        CHECK(cb.back() == 4);
        CHECK(cb.max_size() == defaultMaxSize);
    }

    SUBCASE("construction with single element initializer list") {
        CircularBuffer<int> cb{1};
        CHECK(cb.size() == 1);
        CHECK(cb.front() == 1);
        CHECK(cb.back() == 1);
        CHECK(cb.max_size() == defaultMaxSize);
    }

    SUBCASE("construction with range") {
        std::vector<int> vec{1, 2, 3, 4};
        CircularBuffer<int> cb(vec);
        CHECK(cb.size() == 4);
        CHECK(cb.front() == 1);
        CHECK(cb.back() == 4);
        CHECK(cb.max_size() == defaultMaxSize);
    }

    SUBCASE("construction with rvalue range moves elements") {
        int move_count = 0;
        struct MoveTracker
        {
            int value;
            int* move_count_ptr;
            MoveTracker(int v, int* move_count_ptr) : value(v), move_count_ptr(move_count_ptr) {}
            MoveTracker(MoveTracker const&) = delete;
            MoveTracker(MoveTracker&& other) noexcept : value(other.value), move_count_ptr(other.move_count_ptr) { ++(*move_count_ptr); }
            bool operator==(int other) const { return value == other; }
        };

        std::vector<MoveTracker> vec;
        vec.reserve(3);
        vec.emplace_back(1, &move_count);
        vec.emplace_back(2, &move_count);
        vec.emplace_back(3, &move_count);
        CHECK(move_count == 0);
        CircularBuffer<MoveTracker> cb(std::move(vec));
        CHECK(move_count == 3);
        CHECK(cb.size() == 3);
        CHECK(cb[0] == 1);
        CHECK(cb[1] == 2);
        CHECK(cb[2] == 3);
    }

    SUBCASE("construction with lvalue range copies elements") {
        int copy_count = 0;
        struct CopyTracker
        {
            int value;
            int* copy_count_ptr;
            CopyTracker(int v, int* copy_count_ptr) : value(v), copy_count_ptr(copy_count_ptr) {}
            CopyTracker(CopyTracker&&) = delete;
            CopyTracker(CopyTracker const& other) noexcept : value(other.value), copy_count_ptr(other.copy_count_ptr) { ++(*copy_count_ptr); }
            bool operator==(int other) const { return value == other; }
        };

        std::array<CopyTracker, 3> arr{
            CopyTracker(1, &copy_count),
            CopyTracker(2, &copy_count),
            CopyTracker(3, &copy_count),
        };
        CHECK(copy_count == 0);
        CircularBuffer<CopyTracker> cb(arr);
        CHECK(copy_count == 3);
        CHECK(cb.size() == 3);
        CHECK(cb[0] == 1);
        CHECK(cb[1] == 2);
        CHECK(cb[2] == 3);
    }

    SUBCASE("construction with initializer list and max size") {
        CircularBuffer<int> cb({1, 2, 3}, 8);
        CHECK(cb.size() == 3);
        CHECK(cb.front() == 1);
        CHECK(cb.back() == 3);
        CHECK(cb.max_size() == 8);
    }

    SUBCASE("construction with empty initializer list") {
        CircularBuffer<int> cb{};
        CHECK(cb.empty());
        CHECK(cb.max_size() == defaultMaxSize);
    }

#if not ARC_COMPILER_LT(GCC, 15)
    SUBCASE("constructor type deduction with initializer list") {
        CircularBuffer cb{1, 2, 3}; // deduce CircularBuffer<int>
        CHECK(cb.size() == 3);
        CHECK(cb[0] == 1);
        CHECK(cb[1] == 2);
        CHECK(cb[2] == 3);
    }

    SUBCASE("constructor type deduction with range") {
        std::vector vec{10, 20, 30};
        CircularBuffer cb(vec); // deduce CircularBuffer<int>
        CHECK(cb.size() == 3);
        CHECK(cb[0] == 10);
        CHECK(cb[1] == 20);
        CHECK(cb[2] == 30);
    }
#endif
}

TEST_CASE("CircularBuffer push_back and basic access") {

    SUBCASE("push_back single element") {
        CircularBuffer<int> cb(64);
        cb.push_back(42);
        CHECK(!cb.empty());
        CHECK(cb.size() == 1);
        CHECK(cb.capacity() == 16); // jumps straight to 16
        CHECK(cb.front() == 42);
        CHECK(cb.back() == 42);
        CHECK(cb[0] == 42);
        CHECK(cb.at(0) == 42);
    }

    SUBCASE("push_back multiple elements") {
        CircularBuffer<int> cb(8);
        for (int i = 0; i < 5; ++i) {
            cb.push_back(i);
            CHECK(cb.capacity() == 8); // jumps straight to 8 (constrained by max_size)
        }
        CHECK(cb.size() == 5);
        CHECK(cb.front() == 0);
        CHECK(cb.back() == 4);
        for (int i = 0; i < 5; ++i) {
            CHECK(cb[i] == i);
        }
    }

    SUBCASE("at out of range") {
        CircularBuffer<int> cb(8);
        cb.push_back(1);
        CHECK_THROWS_AS(cb.at(1), std::out_of_range);
    }
}

TEST_CASE("CircularBuffer pop_front") {
    CircularBuffer<int> cb(8);
    cb.push_back(1);
    cb.push_back(2);
    cb.push_back(3);

    SUBCASE("pop_front") {
        cb.pop_front();
        CHECK(cb.size() == 2);
        CHECK(cb.front() == 2);
        CHECK(cb.back() == 3);
    }

    SUBCASE("pop_front_value") {
        int val = cb.pop_front_value();
        CHECK(val == 1);
        CHECK(cb.size() == 2);
        CHECK(cb.front() == 2);
    }
}

TEST_CASE("CircularBuffer iterators") {
    CircularBuffer<int> cb(8);
    cb.push_back(1);
    cb.push_back(2);
    cb.push_back(3);

    SUBCASE("forward iteration") {
        auto it = cb.begin();
        CHECK(*it == 1);
        ++it;
        CHECK(*it == 2);
        ++it;
        CHECK(*it == 3);
        ++it;
        CHECK(it == cb.end());
    }

    SUBCASE("reverse iteration") {
        auto it = cb.rbegin();
        CHECK(*it == 3);
        ++it;
        CHECK(*it == 2);
        ++it;
        CHECK(*it == 1);
        ++it;
        CHECK(it == cb.rend());
    }

    SUBCASE("random access") {
        CHECK(cb.begin()[0] == 1);
        CHECK(cb.begin()[1] == 2);
        CHECK(cb.begin()[2] == 3);
    }
}

TEST_CASE("CircularBuffer full buffer") {
    CircularBuffer<int> cb(4);
    for (int i = 0; i < 4; ++i) {
        cb.push_back(i);
    }
    CHECK(cb.full());
    CHECK(cb.size() == 4);

    SUBCASE("push_back when full") {
        cb.push_back(4);
        CHECK(cb.size() == 4);
        CHECK(cb.front() == 1); // oldest element evicted
        CHECK(cb.back() == 4);
    }
}

TEST_CASE("CircularBuffer clear") {
    CircularBuffer<int> cb(8);
    cb.push_back(1);
    cb.push_back(2);
    cb.push_back(3);
    CHECK(cb.size() == 3);

    cb.clear();
    CHECK(cb.empty());
    CHECK(cb.size() == 0);
}

TEST_CASE("CircularBuffer reserve") {
    CircularBuffer<int> cb(16);

    SUBCASE("reserve increases capacity") {
        cb.reserve(8);
        CHECK(cb.capacity() == 8);
    }

    SUBCASE("reserve default") {
        cb.reserve(); // max_size is 16, so should cap at 16
        CHECK(cb.capacity() == 16);
    }

    SUBCASE("reserve up to max_size") {
        cb.reserve(32); // max_size is 16, so should cap at 16
        CHECK(cb.capacity() == 16);
    }
}

TEST_CASE("CircularBuffer set_max_size") {
    CircularBuffer<int> cb(8);
    for (int i = 0; i < 6; ++i) {
        cb.push_back(i);
    }

    SUBCASE("increase max_size") {
        cb.set_max_size(16);
        CHECK(cb.max_size() == 16);
        CHECK(cb.size() == 6);
        CHECK(cb.capacity() == 8);
    }

    SUBCASE("decrease max_size") {
        cb.set_max_size(4);
        CHECK(cb.max_size() == 4);
        CHECK(cb.size() == 4);
        CHECK(cb.capacity() == 4);
        CHECK(cb.front() == 2); // oldest 2 elements evicted
    }

    SUBCASE("invalid max_size") {
        CHECK_THROWS_AS(cb.set_max_size(0), std::invalid_argument);
    }
}

TEST_CASE("CircularBuffer copy and move") {
    CircularBuffer<int> cb1(8);
    cb1.push_back(1);
    cb1.push_back(2);
    cb1.push_back(3);
    std::size_t id = cb1.begin_id();

    SUBCASE("copy construction") {
        CircularBuffer<int> cb2(cb1);
        CHECK(cb2.size() == 3);
        CHECK(cb2.front() == 1);
        CHECK(cb2.back() == 3);
        CHECK(cb1.contains_id(id)); // original still valid
    }

    SUBCASE("copy assignment") {
        CircularBuffer<int> cb2{1, 2, 3};
        std::size_t id2 = cb2.begin_id();
        CHECK(cb2.contains_id(id2));
        cb2 = cb1;
        CHECK(cb2.size() == 3);
        CHECK(cb2.front() == 1);
        CHECK(cb2.back() == 3);
        CHECK(cb1.contains_id(id)); // original still valid
        CHECK(not cb2.contains_id(id2)); // copy invalidates previous ids in cb2
    }

    SUBCASE("move construction") {
        CircularBuffer<int> cb2(std::move(cb1));
        CHECK(cb2.size() == 3);
        CHECK(cb2.front() == 1);
        CHECK(cb2.back() == 3);
        CHECK(cb1.empty()); // moved-from is empty
        CHECK(cb1.capacity() == 0); // moved-from has zero capacity
        CHECK(not cb1.contains_id(id)); // ids invalidated in cb1
    }

    SUBCASE("move assignment") {
        CircularBuffer<int> cb2{1, 2, 3};
        std::size_t id2 = cb2.begin_id();
        CHECK(cb2.contains_id(id2));
        cb2 = std::move(cb1);
        CHECK(cb2.size() == 3);
        CHECK(cb2.front() == 1);
        CHECK(cb2.back() == 3);
        CHECK(cb1.empty()); // moved-from is empty
        CHECK(cb1.capacity() == 0); // moved-from has zero capacity
        CHECK(not cb1.contains_id(id)); // ids invalidated in cb1
        CHECK(not cb2.contains_id(id2)); // move invalidates previous ids in cb2
    }
}

TEST_CASE("CircularBuffer swap") {
    CircularBuffer<int> cb1(8);
    cb1.push_back(1);
    cb1.push_back(2);
    cb1.push_back(3);
    std::size_t id1 = cb1.begin_id();

    CircularBuffer<int> cb2(16);
    cb2.push_back(10);
    cb2.push_back(20);
    std::size_t id2 = cb2.begin_id();

    using std::swap;
    swap(cb1, cb2);

    // cb1 now has cb2's original contents
    CHECK(cb1.size() == 2);
    CHECK(cb1.front() == 10);
    CHECK(cb1.back() == 20);
    CHECK(cb1.max_size() == 16);

    // cb2 now has cb1's original contents
    CHECK(cb2.size() == 3);
    CHECK(cb2.front() == 1);
    CHECK(cb2.back() == 3);
    CHECK(cb2.max_size() == 8);

    // Indices should still be valid
    CHECK(cb1.contains_id(id2));
    CHECK(cb1.at_id_unchecked(id2) == 10);

    CHECK(cb2.contains_id(id1));
    CHECK(cb2.at_id_unchecked(id1) == 1);
}

TEST_CASE("CircularBuffer emplace_back") {
    struct TestStruct {
        int a, b;
        TestStruct(int a, int b) : a(a), b(b) {}
    };

    CircularBuffer<TestStruct> cb(4);
    cb.emplace_back(1, 2);
    cb.emplace_back(3, 4);

    CHECK(cb.size() == 2);
    CHECK(cb.front().a == 1);
    CHECK(cb.front().b == 2);
    CHECK(cb.back().a == 3);
    CHECK(cb.back().b == 4);
}

TEST_CASE("CircularBuffer iterator comparison") {
    CircularBuffer<int> cb(8);
    cb.push_back(1);
    cb.push_back(2);

    auto it1 = cb.begin();
    auto it2 = cb.begin();
    CHECK(it1 == it2);

    ++it1;
    CHECK(it1 != it2);
    CHECK(it1 > it2);
}

TEST_CASE("CircularBuffer contains_id and at_id_unchecked") {
    CircularBuffer<int> cb(8);
    cb.push_back(10);
    cb.push_back(20);
    std::size_t id = cb.begin_id();

    CHECK(cb.contains_id(id));
    CHECK(cb.at_id_unchecked(id) == 10);

    cb.pop_front();
    CHECK(!cb.contains_id(id)); // id no longer valid after pop
}

TEST_CASE("CircularBuffer shrink_to_fit") {
    SUBCASE("shrink when capacity > needed") {
        CircularBuffer<int> cb(16);
        cb.push_back(1);
        cb.push_back(2);
        cb.push_back(3);
        CHECK(cb.size() == 3);
        CHECK(cb.capacity() == 16);
        cb.shrink_to_fit();
        CHECK(cb.size() == 3);
        CHECK(cb.capacity() == 4); // bit_ceil(3) = 4
        CHECK(cb[0] == 1);
        CHECK(cb[1] == 2);
        CHECK(cb[2] == 3);
    }

    SUBCASE("no shrink when capacity == needed") {
        CircularBuffer<int> cb(4);
        cb.push_back(1);
        cb.push_back(2);
        cb.push_back(3);
        cb.push_back(4);
        CHECK(cb.size() == 4);
        CHECK(cb.capacity() == 4);
        cb.shrink_to_fit();
        CHECK(cb.size() == 4);
        CHECK(cb.capacity() == 4);
    }

    SUBCASE("empty buffer") {
        CircularBuffer<int> cb(16);
        CHECK(cb.size() == 0);
        CHECK(cb.capacity() == 0);
        cb.shrink_to_fit();
        CHECK(cb.size() == 0);
        CHECK(cb.capacity() == 0);
    }

    SUBCASE("after clear") {
        CircularBuffer<int> cb(16);
        cb.push_back(1);
        cb.push_back(2);
        CHECK(cb.capacity() == 16);
        cb.clear();
        CHECK(cb.size() == 0);
        cb.shrink_to_fit();
        CHECK(cb.size() == 0);
        CHECK(cb.capacity() == 0);
    }
}

TEST_CASE("CircularBuffer comparison operators") {
    SUBCASE("equality with CircularBuffer") {
        CircularBuffer<int> cb1{1, 2, 3};
        CircularBuffer<int> cb2{1, 2, 3};
        CircularBuffer<int> cb3{1, 2, 4};
        CHECK(cb1 == cb2);
        CHECK(cb1 != cb3);
    }

    SUBCASE("equality with same elements") {
        CircularBuffer<int> cb{1, 2, 3};
        std::vector<int> vec{1, 2, 3};
        CHECK(cb == vec);
        CHECK(vec == cb); // if symmetric, but since it's template, test both ways if possible
    }

    SUBCASE("equality with different elements") {
        CircularBuffer<int> cb{1, 2, 3};
        std::vector<int> vec{1, 2, 4};
        CHECK(cb != vec);
    }

    SUBCASE("equality with different sizes") {
        CircularBuffer<int> cb{1, 2, 3};
        std::vector<int> vec{1, 2};
        CHECK(cb != vec);
    }

    SUBCASE("equality with empty") {
        CircularBuffer<int> cb;
        std::vector<int> vec;
        CHECK(cb == vec);
    }

    SUBCASE("spaceship equal") {
        CircularBuffer<int> cb{1, 2, 3};
        std::vector<int> vec{1, 2, 3};
        auto cmp = cb <=> vec;
        CHECK(cmp == 0);
    }

    SUBCASE("spaceship less") {
        CircularBuffer<int> cb{1, 2, 2};
        std::vector<int> vec{1, 2, 3};
        auto cmp = cb <=> vec;
        CHECK(cmp < 0);
    }

    SUBCASE("spaceship greater") {
        CircularBuffer<int> cb{1, 2, 4};
        std::vector<int> vec{1, 2, 3};
        auto cmp = cb <=> vec;
        CHECK(cmp > 0);
    }

    SUBCASE("spaceship with different sizes") {
        CircularBuffer<int> cb{1, 2};
        std::vector<int> vec{1, 2, 3};
        auto cmp = cb <=> vec;
        CHECK(cmp < 0); // shorter is less
    }

    SUBCASE("spaceship with array") {
        CircularBuffer<int> cb{1, 2, 3};
        std::array<int, 3> arr{1, 2, 3};
        auto cmp = cb <=> arr;
        CHECK(cmp == 0);
    }
}

TEST_CASE("CircularBuffer invalid argument exceptions") {
    SUBCASE("constructor with maxSize 0") {
        CHECK_THROWS_AS(CircularBuffer<int>(0), std::invalid_argument);
    }

    SUBCASE("constructor with maxSize 1") {
        CHECK_THROWS_AS(CircularBuffer<int>(1), std::invalid_argument);
    }

    SUBCASE("constructor with maxSize 2 is valid") {
        CircularBuffer<int> cb(2);
        CHECK(cb.max_size() == 2);
    }

    SUBCASE("set_max_size with 0") {
        CircularBuffer<int> cb(8);
        CHECK_THROWS_AS(cb.set_max_size(0), std::invalid_argument);
    }

    SUBCASE("set_max_size with 1") {
        CircularBuffer<int> cb(8);
        CHECK_THROWS_AS(cb.set_max_size(1), std::invalid_argument);
    }

    SUBCASE("set_max_size with 2 is valid") {
        CircularBuffer<int> cb(8);
        cb.push_back(1);
        cb.push_back(2);
        cb.push_back(3);
        CHECK_NOTHROW(cb.set_max_size(2));
        CHECK(cb.max_size() == 2);
        CHECK(cb.size() == 2);
        CHECK(cb.front() == 2); // oldest evicted
        CHECK(cb.back() == 3);
    }

    SUBCASE("range constructor with range exceeding maxSize") {
        std::vector<int> vec{1, 2, 3, 4, 5};
        CHECK_THROWS_AS((CircularBuffer<int>(vec, 4)), std::invalid_argument);
    }
}

TEST_CASE("CircularBuffer exception safety") {
    // Helper struct that throws on construction after N successful constructions
    static int constructionCount;
    static int destructionCount;
    static int throwAfter;

    struct ThrowingType {
        int value;

        ThrowingType(int v) : value(v) {
            if (constructionCount >= throwAfter) {
                throw std::runtime_error("Construction failed");
            }
            ++constructionCount;
        }

        ThrowingType(ThrowingType const& other) : value(other.value) {
            if (constructionCount >= throwAfter) {
                throw std::runtime_error("Copy construction failed");
            }
            ++constructionCount;
        }

        ThrowingType(ThrowingType&& other) noexcept(false) : value(other.value) {
            if (constructionCount >= throwAfter) {
                throw std::runtime_error("Move construction failed");
            }
            ++constructionCount;
        }

        ~ThrowingType() {
            ++destructionCount;
        }

        bool operator==(int other) const { return value == other; }
    };

    auto resetCounters = [](int throwAt = std::numeric_limits<int>::max()) {
        constructionCount = 0;
        destructionCount = 0;
        throwAfter = throwAt;
    };

    SUBCASE("emplace_back exception leaves buffer unchanged") {
        resetCounters();
        CircularBuffer<ThrowingType> cb(8);
        cb.emplace_back(1);
        cb.emplace_back(2);
        CHECK(cb.size() == 2);
        CHECK(constructionCount == 2);

        // Next construction will throw
        throwAfter = 2;
        CHECK_THROWS_AS(cb.emplace_back(3), std::runtime_error);

        // Buffer should be unchanged
        CHECK(cb.size() == 2);
        CHECK(cb.front() == 1);
        CHECK(cb.back() == 2);
    }

    SUBCASE("emplace_back exception during buffer growth leaves buffer unchanged") {
        resetCounters();
        CircularBuffer<ThrowingType> cb(32);

        // Fill to trigger growth
        for (int i = 0; i < 16; ++i) {
            cb.emplace_back(i);
        }
        CHECK(cb.size() == 16);
        CHECK(cb.capacity() == 16);
        int destructionsBefore = destructionCount;

        // Next emplace will trigger growth, throw during move of element 5
        throwAfter = 16 + 5;
        CHECK_THROWS_AS(cb.emplace_back(16), std::runtime_error);

        // The 5 elements that were moved to the new buffer should be destroyed
        // The original 16 elements remain in the old buffer (exception causes new buffer to be discarded)
        CHECK(destructionCount == destructionsBefore + 5);

        // Buffer should still be valid with original elements
        CHECK(cb.size() == 16);
        CHECK(cb.capacity() == 16);
        for (int i = 0; i < 16; ++i) {
            CHECK(cb[i] == i);
        }
    }

    SUBCASE("push_back exception leaves buffer unchanged") {
        resetCounters();
        CircularBuffer<ThrowingType> cb(8);
        {
            ThrowingType t1(1);
            ThrowingType t2(2);
            cb.push_back(t1);
            cb.push_back(t2);
        }
        CHECK(cb.size() == 2);

        // Create t3 first, then set throwAfter to cause copy to fail
        ThrowingType t3(100);
        throwAfter = constructionCount; // Next construction (the copy) will throw
        CHECK_THROWS_AS(cb.push_back(t3), std::runtime_error);

        // Buffer should be unchanged
        CHECK(cb.size() == 2);
    }

    SUBCASE("range constructor exception cleans up") {
        resetCounters();
        std::vector<ThrowingType> vec;
        vec.reserve(5);
        for (int i = 0; i < 5; ++i) {
            vec.emplace_back(i);
        }
        CHECK(constructionCount == 5);
        int destructionsBefore = destructionCount;

        // Throw during 3rd element copy
        throwAfter = constructionCount + 3;
        CHECK_THROWS_AS((CircularBuffer<ThrowingType>(vec)), std::runtime_error);

        // Exactly 3 elements were constructed in the CircularBuffer before the exception,
        // and they should all be destroyed by the Buffer constructor's catch block
        CHECK(destructionCount == destructionsBefore + 3);
    }

    SUBCASE("copy constructor exception cleans up") {
        resetCounters();
        CircularBuffer<ThrowingType> cb1(8);
        cb1.emplace_back(1);
        cb1.emplace_back(2);
        cb1.emplace_back(3);
        CHECK(constructionCount == 3);
        int destructionsBefore = destructionCount;

        // Throw during 2nd element copy
        throwAfter = constructionCount + 2;
        CHECK_THROWS_AS((CircularBuffer<ThrowingType>(cb1)), std::runtime_error);

        // Exactly 2 elements were constructed before the exception,
        // and they should be destroyed by the Buffer constructor's catch block
        CHECK(destructionCount == destructionsBefore + 2);

        // Original should be unchanged
        CHECK(cb1.size() == 3);
        CHECK(cb1[0] == 1);
        CHECK(cb1[1] == 2);
        CHECK(cb1[2] == 3);
    }

    SUBCASE("copy assignment exception leaves target empty and source unchanged") {
        resetCounters();
        CircularBuffer<ThrowingType> cb1(8);
        cb1.emplace_back(1);
        cb1.emplace_back(2);

        CircularBuffer<ThrowingType> cb2(8);
        cb2.emplace_back(10);
        cb2.emplace_back(20);
        cb2.emplace_back(30);
        int destructionsBefore = destructionCount;

        // Throw during copy assignment (after 1 element successfully copied)
        throwAfter = constructionCount + 1;
        CHECK_THROWS_AS((cb2 = cb1), std::runtime_error);

        // cb2's original 3 elements were destroyed before the copy attempt,
        // and the 1 successfully copied element was destroyed by the catch block
        CHECK(destructionCount == destructionsBefore + 3 + 1);

        // cb2 should be empty (cleared before copy, copy failed)
        CHECK(cb2.empty());

        // cb1 should be unchanged
        CHECK(cb1.size() == 2);
        CHECK(cb1[0] == 1);
        CHECK(cb1[1] == 2);
    }

    SUBCASE("reserve exception during growth leaves buffer unchanged") {
        resetCounters();
        CircularBuffer<ThrowingType> cb(32);
        cb.emplace_back(1);
        cb.emplace_back(2);
        cb.emplace_back(3);
        CHECK(cb.capacity() == 16);
        int destructionsBefore = destructionCount;

        // Throw during move in reserve (after 2 elements successfully moved)
        throwAfter = constructionCount + 2;
        CHECK_THROWS_AS(cb.reserve(32), std::runtime_error);

        // The 2 successfully moved elements in new buffer should be destroyed
        // Original elements remain in old buffer (not destroyed)
        CHECK(destructionCount == destructionsBefore + 2);

        // Buffer should be unchanged
        CHECK(cb.size() == 3);
        CHECK(cb.capacity() == 16);
        CHECK(cb[0] == 1);
        CHECK(cb[1] == 2);
        CHECK(cb[2] == 3);
    }

    SUBCASE("set_max_size exception during shrink leaves buffer partially evicted") {
        resetCounters();
        CircularBuffer<ThrowingType> cb(16);
        for (int i = 0; i < 8; ++i) {
            cb.emplace_back(i);
        }
        CHECK(cb.size() == 8);
        CHECK(cb.capacity() == 16);
        int destructionsBefore = destructionCount;

        // set_max_size(4) will first evict 4 oldest elements, then try to move remaining 4
        // Throw during move of 2nd element to new buffer
        throwAfter = constructionCount + 2;
        CHECK_THROWS_AS(cb.set_max_size(4), std::runtime_error);

        // 4 oldest elements were destroyed during eviction,
        // 2 elements were moved to new buffer then destroyed by catch block
        CHECK(destructionCount == destructionsBefore + 4 + 2);

        // Buffer should have 4 remaining elements (the ones that weren't evicted)
        // but capacity unchanged since the move failed
        CHECK(cb.size() == 4);
        CHECK(cb.capacity() == 16);
        CHECK(cb[0] == 4);
        CHECK(cb[1] == 5);
        CHECK(cb[2] == 6);
        CHECK(cb[3] == 7);
    }

    SUBCASE("shrink_to_fit exception leaves buffer unchanged") {
        resetCounters();
        CircularBuffer<ThrowingType> cb(16);
        cb.emplace_back(1);
        cb.emplace_back(2);
        cb.emplace_back(3);
        CHECK(cb.capacity() == 16);
        int destructionsBefore = destructionCount;

        // Throw during move in shrink_to_fit (after 2 elements successfully moved)
        throwAfter = constructionCount + 2;
        CHECK_THROWS_AS(cb.shrink_to_fit(), std::runtime_error);

        // The 2 successfully moved elements in new buffer should be destroyed
        CHECK(destructionCount == destructionsBefore + 2);

        // Buffer should be unchanged (original elements still valid)
        CHECK(cb.size() == 3);
        CHECK(cb.capacity() == 16);
        CHECK(cb[0] == 1);
        CHECK(cb[1] == 2);
        CHECK(cb[2] == 3);
    }

    SUBCASE("pop_front_value exception leaves buffer unchanged") {
        resetCounters();
        CircularBuffer<ThrowingType> cb(8);
        cb.emplace_back(1);
        cb.emplace_back(2);
        CHECK(cb.size() == 2);

        // Throw during move out
        throwAfter = constructionCount;
        CHECK_THROWS_AS(cb.pop_front_value(), std::runtime_error);

        // Element should still be in buffer since move constructor threw before
        // the result object was fully constructed, readIndex was not incremented
        CHECK(cb.size() == 2);
        CHECK(cb.front() == 1);
        CHECK(cb.back() == 2);
    }
}

TEST_CASE("CircularBuffer emplace_back when full with throwing type") {
    static int constructionCount = 0;
    static int destructionCount = 0;
    static int throwAfter = std::numeric_limits<int>::max();

    struct ThrowingType {
        int value;

        ThrowingType(int v) : value(v) {
            if (constructionCount >= throwAfter) {
                throw std::runtime_error("Construction failed");
            }
            ++constructionCount;
        }

        ThrowingType(ThrowingType const& other) : value(other.value) {
            if (constructionCount >= throwAfter) {
                throw std::runtime_error("Copy construction failed");
            }
            ++constructionCount;
        }

        ThrowingType(ThrowingType&& other) noexcept : value(other.value) {}

        ~ThrowingType() {
            ++destructionCount;
        }

        bool operator==(int other) const { return value == other; }
    };

    constructionCount = 0;
    destructionCount = 0;
    throwAfter = std::numeric_limits<int>::max();

    CircularBuffer<ThrowingType> cb(4);
    cb.emplace_back(1);
    cb.emplace_back(2);
    cb.emplace_back(3);
    cb.emplace_back(4);
    CHECK(cb.full());
    CHECK(cb.size() == 4);

    int destructionsBefore = destructionCount;

    // When full, emplace_back destroys front element before constructing new one
    // If construction throws, front element is already destroyed
    throwAfter = constructionCount;
    CHECK_THROWS_AS(cb.emplace_back(5), std::runtime_error);

    // Front element was destroyed before the throw
    CHECK(destructionCount == destructionsBefore + 1);

    // Buffer should have 3 elements now (front was destroyed, new element failed to construct)
    CHECK(cb.size() == 3);
    CHECK(cb.front() == 2);
    CHECK(cb.back() == 4);
}

}
