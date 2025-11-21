#ifndef INCLUDE_ARC_CIRCULAR_BUFFER_HPP
#define INCLUDE_ARC_CIRCULAR_BUFFER_HPP

#include "arc/macros.hpp"

#if !ARC_IMPORT_STD
#include <bit>
#include <concepts>
#include <cstddef>
#include <iterator>
#include <memory>
#include <type_traits>
#include <stdexcept>
#endif

namespace arc {

// A simple circular buffer with a queue interface and random-access iterators
// Standard container and iterator typedefs are provided for compatibility with STL algorithms and ranges.
// UB if more than UINT64_MAX elements have been inserted over its entire lifetime
ARC_MODULE_EXPORT
template<class T>
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
            return parent == other.parent
                ? readIndex <=> other.readIndex
                : std::partial_ordering::unordered;
        }

        bool operator==(Iterator const&) const = default;

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

    explicit CircularBuffer(std::size_t maxSize = 1024)
        : maxSize(std::bit_ceil(maxSize))
    {
        if (maxSize == 0)
            throw std::invalid_argument("CircularBuffer::CircularBuffer: maxSize must be greater than 0");
    }

    template<class Self>
    Iterator<std::is_const_v<Self>> begin(this Self& self)
    {
        return {std::addressof(self), self.readIndex};
    }

    const_iterator cbegin() const
    {
        return begin();
    }

    template<class Self>
    Iterator<std::is_const_v<Self>> end(this Self& self)
    {
        return {std::addressof(self), self.writeIndex};
    }

    const_iterator cend() const
    {
        return end();
    }

    template<class Self>
    auto rbegin(this Self& self)
    {
        return std::reverse_iterator<Iterator<std::is_const_v<Self>>>{self.end()};
    }

    template<class Self>
    auto rend(this Self& self)
    {
        return std::reverse_iterator<Iterator<std::is_const_v<Self>>>{self.begin()};
    }

    T& push_back(T const& value)
    {
        return emplace_back(value);
    }

    T& push_back(T&& value)
    {
        return emplace_back(std::move(value));
    }

    template<class... Args>
    requires std::constructible_from<T, Args...>
    T& emplace_back(Args&&... args)
    {
        std::size_t prevSize = size();
        // If we need to grow the buffer
        if (prevSize == buffer.capacity and buffer.capacity < maxSize) [[unlikely]]
            grow_buffer();

        T* dst = buffer.storage_at(writeIndex);
        // If at maxSize and about to overwrite the front element, discard it first
        if (prevSize == maxSize)
        {
            std::destroy_at(std::launder(dst));
            ++readIndex;
        }
        dst = std::construct_at(dst, std::forward<Args>(args)...);
        // Increment writeIndex after constructing the new element in case of exceptions
        ++writeIndex;
        return *dst;
    }

    T pop_front_value()
    {
        T* p = buffer.value_at(readIndex);
        T result(std::move(*p));
        std::destroy_at(p);
        // Increment readIndex after moving the value in case of exceptions
        ++readIndex;
        return result;
    }

    void pop_front()
    {
        std::destroy_at(buffer.value_at(readIndex));
        ++readIndex;
    }

    auto& front(this auto& self)
    {
        return *self.buffer.value_at(self.readIndex);
    }

    auto& back(this auto& self)
    {
        return *self.buffer.value_at(self.writeIndex - 1);
    }

    auto& operator[](this auto& self, size_type pos)
    {
        return *self.buffer.value_at(self.readIndex + pos);
    }

    auto& at(this auto& self, size_type pos)
    {
        if (pos >= self.size())
            throw std::out_of_range("CircularBuffer::at: index out of range");
        return *self.buffer.value_at(self.readIndex + pos);
    }

    bool empty() const
    {
        return writeIndex == readIndex;
    }

    bool full() const
    {
        return writeIndex - readIndex == maxSize;
    }

    std::size_t size() const
    {
        return writeIndex - readIndex;
    }

    std::size_t max_size() const
    {
        return maxSize;
    }

    std::size_t capacity() const
    {
        return buffer.capacity;
    }

    std::size_t begin_id() const
    {
        return readIndex;
    }

    // Use this with begin_id() or it->id() to see if entries have been evicted since your last access
    bool contains_id(std::size_t id) const
    {
        return id >= readIndex and id < writeIndex;
    }

    auto& at_id_unchecked(std::size_t id)
    {
        return *buffer.value_at(id);
    }

    void clear()
    {
        destroy_values();
        // Align readIndex to the next multiple of capacity (power of 2) after writeIndex
        // Safe even if capacity is 0, as in that case both indices will be set to 0
        readIndex = (writeIndex + buffer.capacity - 1) & ~(buffer.capacity - 1);
        writeIndex = readIndex;
    }

    void reserve()
    {
        if (buffer.capacity < maxSize)
            grow_buffer(maxSize);
    }

    // Does not increase capacity beyond the maximum size set at construction or by set_max_size.
    void reserve(std::size_t newCapacity)
    {
        newCapacity = std::min(newCapacity, maxSize);
        if (buffer.capacity < newCapacity)
            grow_buffer(newCapacity);
    }

    // Preserves writeIndex. Preserves readIndex if it fits in the new maxSize.
    void set_max_size(std::size_t newMaxSize)
    {
        if (newMaxSize == 0)
            throw std::invalid_argument("CircularBuffer::set_max_size: capacity must be greater than 0");
        newMaxSize = std::bit_ceil(newMaxSize);

        // When reducing maxSize, keep the most recent elements (towards the end)
        std::size_t elementsToKeep = std::min(writeIndex - readIndex, newMaxSize);
        std::size_t newReadIndex = writeIndex - elementsToKeep;

        // Destroy oldest elements that don't fit in the new maxSize
        while (readIndex < newReadIndex)
        {
            std::destroy_at(buffer.value_at(readIndex));
            ++readIndex;
        }

        std::size_t newStartPos = readIndex & (newMaxSize - 1);
        buffer = Buffer(std::move(buffer), std::min(newStartPos + elementsToKeep, newMaxSize), readIndex, writeIndex);
        maxSize = newMaxSize;
    }

    CircularBuffer(CircularBuffer&&) = default;
    CircularBuffer(CircularBuffer const& other)
        : buffer{other.buffer, other.buffer.capacity, other.readIndex, other.writeIndex}
        , maxSize{other.maxSize}
        , writeIndex{other.writeIndex}
        , readIndex{other.readIndex}
    {}

    CircularBuffer& operator=(CircularBuffer&&) = default;
    CircularBuffer& operator=(CircularBuffer const& other)
    {
        if (this != &other)
            *this = CircularBuffer(other);
        return *this;
    }

    ~CircularBuffer()
    {
        destroy_values();
    }

private:
    void destroy_values()
    {
        for (std::size_t i = readIndex; i < writeIndex; ++i)
            std::destroy_at(buffer.value_at(i));
    }

    ARC_COLD
    void grow_buffer()
    {
        grow_buffer(std::min(std::max(2 * buffer.capacity, std::size_t{16}), maxSize));
    }

    void grow_buffer(std::size_t newCapacity)
    {
        buffer = Buffer(std::move(buffer), newCapacity, readIndex, writeIndex);
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

        template<class Other>
        requires std::same_as<Buffer, std::remove_cvref_t<Other>>
        explicit Buffer(Other&& other, std::size_t capacity_, std::size_t readIndex, std::size_t writeIndex)
            : capacity(std::bit_ceil(capacity_))
            , data(std::make_unique<Storage[]>(capacity))
        {
            for (; readIndex < writeIndex; ++readIndex)
            {
                auto* src = other.value_at(readIndex);
                T* dst = storage_at(readIndex);
                std::construct_at(dst, std::forward_like<Other>(*src));
                if constexpr (std::is_rvalue_reference_v<Other&&>)
                    std::destroy_at(src);
            }
        }

        std::size_t capacity = 0; // always power of 2
        std::unique_ptr<Storage[]> data;

        template<class Self>
        auto* value_at(this Self& self, std::size_t index)
        {
            return std::launder(self.storage_at(index));
        }
        template<class Self>
        auto* storage_at(this Self& self, std::size_t index)
        {
            using Ptr = std::conditional_t<std::is_const_v<Self>, T const*, T*>;
            return reinterpret_cast<Ptr>(self.data[index & (self.capacity - 1)].bytes);
        }
    };

    Buffer buffer;
    std::size_t maxSize; // always power of 2, can be greater than buffer.capacity
    std::size_t writeIndex = 0;
    std::size_t readIndex = 0;
};

}

#endif // INCLUDE_ARC_CIRCULAR_BUFFER_HPP
