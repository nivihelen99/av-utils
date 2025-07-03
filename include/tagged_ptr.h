#ifndef TAGGED_PTR_H
#define TAGGED_PTR_H

#include <cstdint>
#include <cstddef> // For size_t, uintptr_t
#include <type_traits> // For std::is_pointer, std::remove_pointer

template <typename T, unsigned TagBits>
class TaggedPtr {
public:
    static_assert(TagBits < sizeof(uintptr_t) * 8, "TagBits must be less than the number of bits in a pointer.");
    // The alignment check for T will be done in constructors or methods where T is expected to be complete.

    // Mask to extract the tag
    static constexpr uintptr_t TAG_MASK = (static_cast<uintptr_t>(1) << TagBits) - 1;
    // Mask to extract the pointer
    static constexpr uintptr_t POINTER_MASK = ~TAG_MASK;

private:
    // Helper to perform the alignment check.
    // This can be called from constructors or methods once T is complete.
    static constexpr void check_alignment() {
        static_assert(alignof(T) >= (static_cast<std::size_t>(1) << TagBits),
                      "Insufficient pointer alignment for the requested number of tag bits. "
                      "alignof(T) must be >= 2^TagBits.");
    }

public:
    TaggedPtr() : data_(0) {
        // Check alignment when a TaggedPtr object is created.
        // This ensures T is complete at this point for non-global/static objects.
        if constexpr (TagBits > 0) { // Avoid check if no bits are used for tag.
            check_alignment();
        }
    }

    TaggedPtr(T* ptr, uint8_t tag) : data_(0) {
        if constexpr (TagBits > 0) { check_alignment(); }
        set(ptr, tag);
    }

    void set(T* ptr, uint8_t tag) {
        // Optional: runtime check for actual pointer alignment if desired, e.g., assert((reinterpret_cast<uintptr_t>(ptr) & TAG_MASK) == 0);
        uintptr_t ptr_val = reinterpret_cast<uintptr_t>(ptr);
        data_ = (ptr_val & POINTER_MASK) | (static_cast<uintptr_t>(tag) & TAG_MASK);
    }

    void set_ptr(T* ptr) {
        // Optional: runtime check for actual pointer alignment
        uintptr_t ptr_val = reinterpret_cast<uintptr_t>(ptr);
        data_ = (ptr_val & POINTER_MASK) | (data_ & TAG_MASK);
    }

    void set_tag(uint8_t tag) {
        data_ = (data_ & POINTER_MASK) | (static_cast<uintptr_t>(tag) & TAG_MASK);
    }

    T* get_ptr() const {
        return reinterpret_cast<T*>(data_ & POINTER_MASK);
    }

    uint8_t get_tag() const {
        return static_cast<uint8_t>(data_ & TAG_MASK);
    }

    uintptr_t as_uintptr_t() const {
        return data_;
    }

    static TaggedPtr from_raw(uintptr_t raw) {
        // Check alignment when from_raw is called, as T should be known and complete here.
        if constexpr (TagBits > 0) { check_alignment(); }
        TaggedPtr p;
        p.data_ = raw;
        return p;
    }

    bool operator==(const TaggedPtr& other) const {
        return data_ == other.data_;
    }

    bool operator!=(const TaggedPtr& other) const {
        return data_ != other.data_;
    }

    static constexpr size_t max_tag() {
        return (static_cast<size_t>(1) << TagBits) - 1;
    }

private:
    uintptr_t data_;
};

#endif // TAGGED_PTR_H
