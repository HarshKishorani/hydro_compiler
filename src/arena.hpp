#pragma once

#include <cstddef>
#include <cstdlib>
#include <memory>
#include <utility>

/**
 * @class ArenaAllocator
 * @brief A simple arena allocator for memory allocation.
 *
 * This class provides a basic arena allocator that allocates a fixed amount of memory upfront
 * and then doles out memory blocks as requested. It is useful in scenarios where many small
 * allocations and deallocations are needed, as it avoids the overhead of frequent allocations
 * from the heap.
 */
class ArenaAllocator final
{
public:
    /**
     * @brief Constructs the arena allocator with a specified size.
     *
     * @param max_num_bytes The size of the memory buffer to allocate.
     */
    explicit ArenaAllocator(const std::size_t max_num_bytes)
        : m_size{max_num_bytes}, m_buffer{new std::byte[max_num_bytes]}, m_offset{m_buffer}
    {
        // Initialize the buffer with the specified size and set the offset to the start of the buffer.
    }

    /**
     * @brief Deleted copy constructor to prevent copying.
     *
     * Copying an ArenaAllocator is not allowed as it could lead to double freeing of memory
     * or other undefined behavior.
     */
    ArenaAllocator(const ArenaAllocator &) = delete;

    /**
     * @brief Deleted copy assignment operator to prevent copying.
     *
     * Copy assignment is also deleted for the same reasons as the copy constructor.
     */
    ArenaAllocator &operator=(const ArenaAllocator &) = delete;

    /**
     * @brief Move constructor.
     *
     * Transfers ownership of the internal buffer and other resources from one ArenaAllocator
     * to another, ensuring that the original allocator is left in a valid but empty state.
     *
     * @param other The allocator to move from.
     */
    ArenaAllocator(ArenaAllocator &&other) noexcept
        : m_size{std::exchange(other.m_size, 0)}, m_buffer{std::exchange(other.m_buffer, nullptr)}, m_offset{std::exchange(other.m_offset, nullptr)}
    {
        // Exchange the resources from the source allocator to this allocator.
    }

    /**
     * @brief Move assignment operator.
     *
     * Transfers ownership of the internal buffer and other resources from one ArenaAllocator
     * to another, ensuring that the original allocator is left in a valid but empty state.
     *
     * @param other The allocator to move from.
     * @return A reference to the current object.
     */
    ArenaAllocator &operator=(ArenaAllocator &&other) noexcept
    {
        // Swap the resources with the other allocator.
        std::swap(m_size, other.m_size);
        std::swap(m_buffer, other.m_buffer);
        std::swap(m_offset, other.m_offset);
        return *this;
    }

    /**
     * @brief Allocates memory for an object of type T.
     *
     * This function allocates memory from the internal buffer for an object of type T.
     * The memory is aligned to the alignment requirements of T.
     *
     * @tparam T The type of the object to allocate memory for.
     * @return A pointer to the allocated memory for the object of type T.
     * @throws std::bad_alloc if there is not enough memory left in the buffer.
     */
    template <typename T>
    [[nodiscard]] T *alloc()
    {
        // Calculate the remaining number of bytes in the buffer.
        std::size_t remaining_num_bytes = m_size - static_cast<std::size_t>(m_offset - m_buffer);

        // Attempt to align the memory for the type T.
        auto pointer = static_cast<void *>(m_offset);
        const auto aligned_address = std::align(alignof(T), sizeof(T), pointer, remaining_num_bytes);

        // If alignment fails, throw an exception.
        if (aligned_address == nullptr)
        {
            throw std::bad_alloc{};
        }

        // Move the offset forward by the size of T.
        m_offset = static_cast<std::byte *>(aligned_address) + sizeof(T);
        return static_cast<T *>(aligned_address);
    }

    /**
     * @brief Constructs an object of type T in place.
     *
     * This function allocates memory for an object of type T and constructs it in place using
     * the provided arguments.
     *
     * @tparam T The type of the object to construct.
     * @tparam Args The types of the arguments to pass to the constructor of T.
     * @param args The arguments to pass to the constructor of T.
     * @return A pointer to the constructed object of type T.
     * @throws std::bad_alloc if there is not enough memory left in the buffer.
     */
    template <typename T, typename... Args>
    [[nodiscard]] T *emplace(Args &&...args)
    {
        // Allocate memory for the object.
        const auto allocated_memory = alloc<T>();

        // Construct the object in place using the provided arguments.
        return new (allocated_memory) T{std::forward<Args>(args)...};
    }

    /**
     * @brief Destructor for the ArenaAllocator.
     *
     * The destructor releases the allocated memory buffer. Note that no destructors are called
     * for objects allocated in the buffer, which may lead to memory leaks if objects with non-trivial
     * destructors (e.g., std::vector) are used. This is a trade-off for performance and simplicity.
     */
    ~ArenaAllocator()
    {
        delete[] m_buffer; // Release the allocated memory buffer.
    }

private:
    std::size_t m_size;  // The size of the memory buffer.
    std::byte *m_buffer; // The allocated memory buffer.
    std::byte *m_offset; // The current offset within the buffer, indicating the next free memory location.
};
