#pragma once

/**
 * @class ArenaAllocator
 * @brief A simple arena allocator for memory allocation.
 *
 * This class provides a basic arena allocator that allocates a fixed amount of memory upfront
 * and then doles out memory blocks as requested.
 */
class ArenaAllocator
{
public:
    /**
     * @brief Constructs the arena allocator with a specified size.
     *
     * @param bytes The size of the memory buffer to allocate.
     */
    inline explicit ArenaAllocator(size_t bytes) : m_size(bytes)
    {
        // Allocate the buffer of specified size
        m_buffer = static_cast<std::byte *>(malloc(m_size));
        // Initialize the offset pointer to the start of the buffer
        m_offset = m_buffer;
    }

    /**
     * @brief Allocates memory for an object of type T.
     *
     * @tparam T The type of the object to allocate.
     * @return A pointer to the allocated memory for an object of type T.
     */
    template <typename T>
    inline T *alloc()
    {
        // Store the current offset
        void *offset = m_offset;
        // Move the offset pointer forward by the size of T
        m_offset += sizeof(T);
        // Return the allocated memory cast to type T
        return static_cast<T *>(offset);
    }

    /**
     * @brief Deleted copy constructor to prevent copying.
     */
    inline ArenaAllocator(const ArenaAllocator &other) = delete;

    /**
     * @brief Deleted copy assignment operator to prevent copying.
     */
    inline ArenaAllocator operator=(const ArenaAllocator &other) = delete;

    /**
     * @brief Destructor to free the allocated buffer.
     */
    inline ~ArenaAllocator()
    {
        // Free the allocated buffer
        free(m_buffer);
    }

private:
    size_t m_size;       // The size of the memory buffer
    std::byte *m_buffer; // The allocated memory buffer
    std::byte *m_offset; // The current offset within the buffer
};
