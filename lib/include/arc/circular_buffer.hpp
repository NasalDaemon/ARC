#ifndef INCLUDE_ARC_CIRCULAR_BUFFER_HPP
#define INCLUDE_ARC_CIRCULAR_BUFFER_HPP

#include "arc/macros.hpp"
#include "arc/detail/concepts.hpp"

#if !ARC_IMPORT_STD
#include <algorithm>
#include <bit>
#include <compare>
#include <concepts>
#include <cstddef>
#include <initializer_list>
#include <iterator>
#include <memory>
#include <optional>
#include <ranges>
#include <stdexcept>
#ifndef NDEBUG
#include <string>
#endif
#include <type_traits>
#include <utility>
#endif

namespace arc {

// A simple circular buffer with a queue interface and random-access iterators
// Standard container and iterator typedefs are provided for compatibility with STL algorithms and ranges.
ARC_MODULE_EXPORT
template<class T, bool Growing = true>
struct CircularBuffer
{
    // Standard container typedefs
    using value_type = T;
    using reference = T&;
    using const_reference = T const&;
    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;

    template<bool Const>
    struct Iterator
    {
        using iterator_category = std::random_access_iterator_tag;
        using value_type = T;
        using difference_type = std::ptrdiff_t;
        using pointer = std::conditional_t<Const, T const*, T*>;
        using reference = std::conditional_t<Const, T const&, T&>;

        Iterator() = default;

        template<bool OtherConst>
        requires Const or (not OtherConst)
        Iterator(Iterator<OtherConst> const& other)
            : parent(other.parent)
            , readIndex(other.readIndex)
        {}

        reference operator*() const
        {
            return *parent->buffer.value_at(readIndex);
        }

        pointer operator->() const
        {
            return parent->buffer.value_at(readIndex);
        }

        reference operator[](difference_type n) const
        {
            return *parent->buffer.value_at(readIndex + n);
        }

        Iterator& operator++()
        {
            readIndex++;
            return *this;
        }

        Iterator operator++(int)
        {
            Iterator tmp = *this;
            ++(*this);
            return tmp;
        }

        Iterator& operator--()
        {
            readIndex--;
            return *this;
        }

        Iterator operator--(int)
        {
            Iterator tmp = *this;
            --(*this);
            return tmp;
        }

        Iterator& operator+=(difference_type n)
        {
            readIndex += n;
            return *this;
        }

        Iterator& operator-=(difference_type n)
        {
            readIndex -= n;
            return *this;
        }

        Iterator operator+(difference_type n) const
        {
            Iterator tmp = *this;
            tmp += n;
            return tmp;
        }

        Iterator operator-(difference_type n) const
        {
            Iterator tmp = *this;
            tmp -= n;
            return tmp;
        }

        friend Iterator operator+(difference_type n, Iterator const& it)
        {
            return it + n;
        }

        difference_type operator-(Iterator const& other) const
        {
            return static_cast<difference_type>(readIndex) - static_cast<difference_type>(other.readIndex);
        }

        constexpr auto operator<=>(Iterator const& other) const
        {
            // Use offset from parent's readIndex to handle index wraparound
            return parent == other.parent
                ? (readIndex - parent->readIndex) <=> (other.readIndex - parent->readIndex)
                : std::partial_ordering::unordered;
        }

        constexpr bool operator==(Iterator const& other) const
        {
            return parent == other.parent and readIndex == other.readIndex;
        }

        constexpr std::size_t id() const
        {
            return readIndex;
        }

        constexpr bool is_valid_id() const
        {
            return parent->contains_id(id());
        }

    private:
        friend struct CircularBuffer;

        using Parent = std::conditional_t<Const, CircularBuffer const, CircularBuffer>;

        Iterator(Parent* parent, std::size_t index)
            : parent(parent)
            , readIndex(index)
        {}

        Parent* parent = nullptr;
        std::size_t readIndex = 0;
    };

    using iterator = Iterator<false>;
    using const_iterator = Iterator<true>;
    using reverse_iterator = std::reverse_iterator<iterator>;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;

    static constexpr std::size_t defaultMaxSize = 1024;

    CircularBuffer() = default;

    explicit constexpr CircularBuffer(std::size_t maxSize_ /*= defaultMaxSize*/)
    {
        if (maxSize_ < 2)
            throw std::invalid_argument("CircularBuffer::CircularBuffer: maxSize must be greater than 1");

        std::size_t const mask = mask_for(maxSize_);
        maxSize = capacity_of_mask(mask);
        if constexpr (not Growing)
        {
            buffer.data = std::make_unique<Storage[]>(maxSize);
            buffer.mask = mask;
        }
    }

    template<std::ranges::input_range R>
    requires std::ranges::sized_range<R>
         and (not std::same_as<std::remove_cvref_t<R>, CircularBuffer>)
         and std::constructible_from<T, decltype(std::forward_like<R>(std::declval<std::ranges::range_reference_t<R>>()))>
    constexpr CircularBuffer(R&& range, std::optional<std::size_t> maxSize_ = std::nullopt)
        : CircularBuffer(maxSize_ ? *maxSize_ : std::max(defaultMaxSize, std::ranges::size(range)))
    {
        std::size_t const size = std::ranges::size(range);
        if (size == 0)
            return;
        if (maxSize_ and size > *maxSize_)
            throw std::invalid_argument("CircularBuffer::CircularBuffer: range size exceeds provided maxSize");

        // If Growing, buffer has not been allocated
        if constexpr (Growing)
        {
            std::size_t const mask = min_mask(mask_for(size));
            buffer.data = std::make_unique<Storage[]>(capacity_of_mask(mask));
            buffer.mask = mask;
        }

        T* outStorage = reinterpret_cast<T*>(buffer.data.get());
        for (auto&& elem : ARC_FWD(range))
        {
            std::construct_at(outStorage + writeIndex, std::forward_like<R>(elem));
            ++writeIndex;
        }
    }

    constexpr CircularBuffer(std::initializer_list<T> init, std::optional<std::size_t> maxSize_ = std::nullopt) requires detail::CopyConstructible<T>
        : CircularBuffer(std::ranges::subrange(init), maxSize_)
    {}

    constexpr CircularBuffer(CircularBuffer const& other) requires detail::CopyConstructible<T>
        : buffer{other.buffer, other.buffer.mask, other.readIndex, other.writeIndex, 0}
        , maxSize{other.maxSize}
        , readIndex{0}
        , writeIndex{other.writeIndex - other.readIndex}
    {}

    constexpr CircularBuffer(CircularBuffer&& other) noexcept
        : buffer(std::move(other.buffer))
        , maxSize(other.maxSize)
        , readIndex(other.readIndex)
        , writeIndex(other.writeIndex)
    {
        other.readIndex = other.writeIndex;
    }

    constexpr CircularBuffer& operator=(CircularBuffer const& other) requires detail::CopyConstructible<T>
    {
        if (this != &other)
        {
            destroy_values();
            readIndex = writeIndex;
            buffer = Buffer{other.buffer, other.buffer.mask, other.readIndex, other.writeIndex, readIndex};
            writeIndex += (other.writeIndex - other.readIndex);
            maxSize = other.maxSize;
        }
        return *this;
    }

    constexpr CircularBuffer& operator=(CircularBuffer&& other) noexcept
    {
        if (this != &other)
        {
            destroy_values();
            readIndex = writeIndex; // Invalidate all old IDs
            buffer = std::move(other.buffer);
            maxSize = other.maxSize;
            if (buffer.mask != 0)
            {
                // Set readIndex to the next multiple of capacity (power of 2) after writeIndex
                // This ensures old IDs are invalidated even with index wraparound
                readIndex = (writeIndex + buffer.mask) & ~buffer.mask;
                // Set readIndex to the same offset within the buffer as other.readIndex
                readIndex += other.readIndex & buffer.mask;
                writeIndex = readIndex + (other.writeIndex - other.readIndex);
            }
            other.readIndex = other.writeIndex;
        }
        return *this;
    }

    template<std::ranges::range R>
    requires std::equality_comparable_with<T, std::ranges::range_value_t<R>>
    constexpr bool operator==(R const& other) const
    {
        return std::ranges::equal(*this, other);
    }

    template<std::ranges::range R>
    requires std::three_way_comparable_with<T, std::ranges::range_value_t<R>>
    constexpr auto operator<=>(R const& other) const
    {
        return std::lexicographical_compare_three_way(begin(), end(), other.begin(), other.end());
    }

    constexpr void swap(CircularBuffer& other) noexcept
    {
        std::swap(buffer.mask, other.buffer.mask);
        std::swap(buffer.data, other.buffer.data);
        std::swap(maxSize, other.maxSize);
        std::swap(readIndex, other.readIndex);
        std::swap(writeIndex, other.writeIndex);
    }

    friend constexpr void swap(CircularBuffer& lhs, CircularBuffer& rhs) noexcept
    {
        lhs.swap(rhs);
    }

    template<class Self>
    constexpr Iterator<std::is_const_v<Self>> begin(this Self& self) noexcept
    {
        return {std::addressof(self), self.readIndex};
    }

    constexpr const_iterator cbegin() const noexcept
    {
        return begin();
    }

    template<class Self>
    constexpr Iterator<std::is_const_v<Self>> end(this Self& self) noexcept
    {
        return {std::addressof(self), self.writeIndex};
    }

    constexpr const_iterator cend() const noexcept
    {
        return end();
    }

    template<class Self>
    constexpr auto rbegin(this Self& self) noexcept
    {
        return std::reverse_iterator<Iterator<std::is_const_v<Self>>>{self.end()};
    }

    template<class Self>
    constexpr auto rend(this Self& self) noexcept
    {
        return std::reverse_iterator<Iterator<std::is_const_v<Self>>>{self.begin()};
    }

    constexpr T& push_back(T const& value) requires detail::CopyConstructible<T>
    {
        return emplace_back(value);
    }

    constexpr T& push_back(T&& value) requires detail::MoveConstructible<T>
    {
        return emplace_back(std::move(value));
    }

    template<class... Args>
    requires std::constructible_from<T, Args...>
    constexpr T& emplace_back(Args&&... args)
    {
        std::size_t const prevSize = size();
        // If at maxSize and about to overwrite the front element, discard it first.
        if (prevSize == maxSize) [[unlikely]]
        {
            std::destroy_at(buffer.value_at(readIndex));
            ++readIndex;
        }
        else if constexpr (Growing)
        {
            // If we need to grow the buffer and we can (also works when prevSize == 0)
            if (buffer.mask < (prevSize | 1)) [[unlikely]]
                grow_buffer();
        }

        T* dst = std::construct_at(buffer.storage_at(writeIndex), std::forward<Args>(args)...);
        // Increment writeIndex after constructing the new element in case of exceptions
        ++writeIndex;
        return *dst;
    }

    constexpr T pop_front_value() requires detail::MoveConstructible<T>
    {
        T* p = buffer.value_at(readIndex);
        T result(std::move(*p));
        std::destroy_at(p);
        // Discard element after moving the value in case of exceptions
        ++readIndex;
        return result;
    }

    constexpr void pop_front() noexcept
    {
        std::destroy_at(buffer.value_at(readIndex));
        ++readIndex;
    }

    constexpr auto& front(this auto& self) noexcept
    {
        return *self.buffer.value_at(self.readIndex);
    }

    constexpr auto& back(this auto& self) noexcept
    {
        return *self.buffer.value_at(self.writeIndex - 1);
    }

    constexpr auto& operator[](this auto& self, size_type pos) noexcept
    {
        return *self.buffer.value_at(self.readIndex + pos);
    }

    constexpr auto& at(this auto& self, size_type pos)
    {
        if (pos >= self.size())
            throw std::out_of_range("CircularBuffer::at: index out of range");
        return *self.buffer.value_at(self.readIndex + pos);
    }

    constexpr bool empty() const noexcept
    {
        return writeIndex == readIndex;
    }

    constexpr bool full() const noexcept
    {
        return size() == max_size();
    }

    constexpr std::size_t size() const noexcept
    {
        return writeIndex - readIndex;
    }

    constexpr std::size_t max_size() const noexcept
    {
        return maxSize;
    }

    constexpr std::size_t capacity() const noexcept
    {
        return capacity_of_mask(buffer.mask);
    }

    constexpr std::size_t begin_id() const noexcept
    {
        return readIndex;
    }

    constexpr std::size_t end_id() const noexcept
    {
        return writeIndex;
    }

    // Use this with begin_id() or it->id() to see if entries have been evicted since your last access
    constexpr bool contains_id(std::size_t id) const noexcept
    {
        // Use subtraction to handle index wraparound correctly
        return (id - readIndex) < size();
    }

    constexpr auto& at_id(this auto& self, std::size_t id)
    {
        if (not self.contains_id(id))
            throw std::out_of_range("CircularBuffer::at_id: id out of range");
        return self.at_id_unchecked(id);
    }

    constexpr auto& at_id_unchecked(this auto& self, std::size_t id) noexcept
    {
        return *self.buffer.value_at(id);
    }

    constexpr void clear() noexcept
    {
        destroy_values();
        readIndex = writeIndex;
    }

    constexpr void shrink_to_fit() requires Growing and detail::MoveConstructible<T>
    {
        std::size_t currentSize = size();
        if (currentSize == 0)
        {
            buffer.data.reset();
            buffer.mask = 0;
        }
        else
        {
            std::size_t neededMask = mask_for(currentSize);
            if (neededMask < buffer.mask)
                buffer = Buffer(std::move(buffer), neededMask, readIndex, writeIndex, readIndex);
        }
    }

    constexpr void reserve() requires Growing and detail::MoveConstructible<T>
    {
        std::size_t maxMask = max_mask();
        if (buffer.mask < maxMask)
            grow_buffer(maxMask);
    }

    // Does not increase capacity beyond the maximum size set at construction or by set_max_size.
    constexpr void reserve(std::size_t newCapacity) requires Growing and detail::MoveConstructible<T>
    {
        std::size_t newMask = mask_for(std::min(newCapacity, maxSize));
        if (buffer.mask < newMask)
            grow_buffer(newMask);
    }

    // Preserves writeIndex. Preserves readIndex if it fits in the new maxSize.
    constexpr void set_max_size(std::size_t newMaxSize) requires detail::MoveConstructible<T>
    {
        if (newMaxSize < 2)
            throw std::invalid_argument("CircularBuffer::set_max_size: max size must be greater than 1");
        std::size_t const newMask = mask_for(newMaxSize);
        maxSize = capacity_of_mask(newMask);

        // If newMaxSize must shrink the capacity, we may need to evict some elements
        if (buffer.mask > newMask)
        {
            newMaxSize = newMask + 1; // we know that newMask is not 0 or UINTMAX here

            // When reducing maxSize, keep the most recent elements (towards the end)
            std::size_t elementsToKeep = std::min(writeIndex - readIndex, newMaxSize);
            std::size_t newReadIndex = writeIndex - elementsToKeep;

            // Destroy oldest elements that don't fit in the new maxSize
            while (readIndex != newReadIndex)
            {
                std::destroy_at(buffer.value_at(readIndex));
                ++readIndex;
            }

            buffer = Buffer(std::move(buffer), newMask, readIndex, writeIndex, readIndex);
        }
        else if constexpr (not Growing)
        {
            // If not growing, we must ensure the buffer is fully allocated up to maxSize
            if (buffer.mask < newMask)
                grow_buffer(newMask);
        }
    }

    constexpr ~CircularBuffer()
    {
        destroy_values();
    }

private:
    constexpr std::size_t max_mask() const noexcept
    {
        // Mask is maxSize - 1, except when maxSize is UINTMAX
        // minimum maxSize is 2, so minimum max_mask is 1
        // maximum maxSize is UINTMAX, so maximum max_mask is UINTMAX
        return (maxSize - 1) | 1;
    }

    constexpr std::size_t min_mask(std::size_t newMask) const
    {
        static constexpr std::size_t minMask = mask_for(16);
        return std::min(std::max(newMask, minMask), max_mask());
    }

    constexpr void destroy_values() noexcept
    {
        // Use != instead of < to handle index wraparound
        for (std::size_t i = readIndex; i != writeIndex; ++i)
            std::destroy_at(buffer.value_at(i));
    }

    ARC_COLD
    constexpr void grow_buffer() requires Growing
    {
        grow_buffer(min_mask((buffer.mask << 1) | 1));
    }

    constexpr void grow_buffer(std::size_t newMask)
    {
        buffer = Buffer(std::move(buffer), newMask, readIndex, writeIndex, readIndex);
    }

    struct alignas(T) Storage
    {
        union
        {
            std::byte bytes[sizeof(T)];
        };
    };

    struct Buffer
    {
        // No allocation when default-constructed
        Buffer() = default;

        constexpr Buffer(Buffer const&) = delete;
        constexpr Buffer(Buffer&& other) noexcept
            : mask(std::exchange(other.mask, 0))
            , data(std::move(other.data))
        {}

        Buffer& operator=(Buffer const&) = delete;
        constexpr Buffer& operator=(Buffer&& other) noexcept
        {
            mask = std::exchange(other.mask, 0);
            data = std::move(other.data);
            return *this;
        }

        template<class Other>
        requires std::same_as<Buffer, std::remove_cvref_t<Other>>
        constexpr explicit Buffer(Other&& other, std::size_t mask, std::size_t readIndex, std::size_t writeIndex, std::size_t newReadIndex)
            : mask(mask)
        {
            std::size_t const capacity = capacity_of_mask(mask);
            std::size_t const size = writeIndex - readIndex;
            if (size > capacity) [[unlikely]]
                throw std::invalid_argument("CircularBuffer::Buffer: not enough capacity to hold all elements");
            if (capacity == 0)
                return;

            data = std::make_unique<Storage[]>(capacity);
            std::size_t index = 0;
            try
            {
                for (; index < size; ++index)
                {
                    auto* src = other.value_at(readIndex + index);
                    T* dst = storage_at(newReadIndex + index);
                    std::construct_at(dst, std::forward_like<Other>(*src));
                }
            }
            catch(...)
            {
                // Destroy any elements that were already constructed in the new buffer
                for (std::size_t i = 0; i < index; ++i)
                {
                    std::destroy_at(value_at(newReadIndex + i));
                }
                throw;
            }
            // Destroy moved-from elements in the old buffer if Other is an rvalue
            // after successfully constructing *all* elements in the new buffer
            if constexpr (std::is_rvalue_reference_v<Other&&>)
            {
                // Use != instead of < to handle index wraparound
                for (; readIndex != writeIndex; ++readIndex)
                {
                    std::destroy_at(other.value_at(readIndex));
                }
            }
        }

        std::size_t mask = 0; // always {power of 2} - 1
        std::unique_ptr<Storage[]> data;

        constexpr auto* value_at(this auto& self, std::size_t index) noexcept
        {
            return std::launder(self.storage_at(index));
        }
        template<class Self>
        constexpr auto* storage_at(this Self& self, std::size_t index) noexcept
        {
            using Ptr = std::conditional_t<std::is_const_v<Self>, T const*, T*>;
            return reinterpret_cast<Ptr>(self.data[index & self.mask].bytes);
        }
    };

    static constexpr std::size_t mask_for(std::size_t requestedCapacity) noexcept
    {
        if (requestedCapacity < 2)
            return requestedCapacity; // 0 => 0 (no capacity), 1 => 1 (capacity: 2)
        return ~std::size_t(0) >> std::countl_zero(requestedCapacity - 1);
    }

    static constexpr std::size_t capacity_of_mask(std::size_t mask) noexcept
    {
        assert_is_mask(mask);
        // Add 1 if mask is not 0 or UINTMAX
        return mask + ((mask << 1) > mask);
    }

#ifdef NDEBUG
    ARC_INLINE static constexpr void assert_is_mask(std::size_t)
    {}
#else
    ARC_INLINE static constexpr void assert_is_mask([[maybe_unused]] std::size_t mask)
    {
        if (mask != 0 and ~mask != 0 and (mask & (mask + 1)) != 0)
            throw std::invalid_argument("CircularBuffer: mask is not of the form 2^n - 1: " + std::to_string(mask));
    }
#endif

    Buffer buffer;
    std::size_t maxSize = defaultMaxSize; // always {power of 2} >= 2 or UINTMAX
    std::size_t readIndex = 0;
    std::size_t writeIndex = 0;
};

ARC_MODULE_EXPORT
template<class T>
using StaticCircularBuffer = CircularBuffer<T, false>;

template<class T>
CircularBuffer(std::initializer_list<T>) -> CircularBuffer<T>;

template<class T>
CircularBuffer(std::initializer_list<T>, std::optional<std::size_t>) -> CircularBuffer<T>;

template<std::ranges::input_range R>
requires std::ranges::sized_range<R>
CircularBuffer(R&&) -> CircularBuffer<std::ranges::range_value_t<R>>;

template<std::ranges::input_range R>
requires std::ranges::sized_range<R>
CircularBuffer(R&&, std::optional<std::size_t>) -> CircularBuffer<std::ranges::range_value_t<R>>;

}

#endif // INCLUDE_ARC_CIRCULAR_BUFFER_HPP
