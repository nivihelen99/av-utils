#pragma once

#include <functional>
#include <optional>
#include <stdexcept> // For std::invalid_argument
#include <utility> // For std::forward, std::move

// Forward declaration for the case where Owner is the class containing CachedProperty
// template<typename T, typename Owner> class CachedProperty;

/**
 * @brief A property whose value is computed once and then cached.
 *
 * Similar to Python's functools.cached_property. It's useful for properties
 * of a class that are expensive to compute and don't change after the first
 * computation.
 *
 * @tparam Owner The type of the class owning this property.
 * @tparam T The type of the property value.
 */
template<typename Owner, typename T>
class CachedProperty {
public:
    using value_type = T;
    using owner_type = Owner;
    using compute_func_type = std::function<T(Owner*)>;

private:
    Owner* owner_ptr_;
    compute_func_type compute_func_;
    mutable std::optional<T> cached_value_; // mutable to allow caching in const contexts if accessed via a const get()

public:
    /**
     * @brief Constructs a CachedProperty.
     * @param owner A pointer to the owning object. This object must outlive the CachedProperty.
     * @param compute_func A function (lambda, member function pointer bound with std::bind, etc.)
     *                     that takes an Owner* and returns the value of type T.
     */
    CachedProperty(Owner* owner, compute_func_type compute_func)
        : owner_ptr_(owner), compute_func_(std::move(compute_func)), cached_value_() {
        if (!owner_ptr_) {
            // Or handle this more gracefully, depending on desired strictness
            throw std::invalid_argument("CachedProperty owner pointer cannot be null.");
        }
        if (!compute_func_) {
            throw std::invalid_argument("CachedProperty compute_func cannot be null.");
        }
    }

    // Prevent copying and moving by default, as ownership and cache state can be tricky.
    // If these are needed, they require careful implementation.
    CachedProperty(const CachedProperty&) = delete;
    CachedProperty& operator=(const CachedProperty&) = delete;
    CachedProperty(CachedProperty&&) = delete;
    CachedProperty& operator=(CachedProperty&&) = delete;

    /**
     * @brief Accesses the property value. Computes it if not already cached.
     * @return A const reference to the cached value.
     */
    const T& get() const {
        if (!cached_value_) {
            cached_value_ = compute_func_(owner_ptr_);
        }
        return *cached_value_;
    }

    /**
     * @brief Conversion operator to access the property value.
     * @return A const reference to the cached value.
     */
    operator const T&() const {
        return get();
    }

    /**
     * @brief Checks if the value has been computed and cached.
     * @return true if the value is cached, false otherwise.
     */
    bool is_cached() const noexcept {
        return cached_value_.has_value();
    }

    /**
     * @brief Clears the cached value, forcing recomputation on next access.
     * Useful if the underlying state of the owner object changes and the
     * property needs to be re-evaluated.
     */
    void invalidate() {
        cached_value_.reset();
    }
};

// Deduction guide (optional, might be complex if Owner type is part of lambda capture)
// For simplicity, explicit template arguments for Owner and T are recommended.

/**
 * @brief Helper function to create a CachedProperty, simplifying template deduction for the callable.
 *
 * @tparam Owner Type of the owning class.
 * @tparam T Return type of the compute function (deduced).
 * @tparam Callable Type of the compute function (deduced).
 * @param owner Pointer to the owning object.
 * @param callable The function/lambda to compute the value.
 * @return A CachedProperty instance.
 *
 * Example:
 * struct MyClass {
 *     int data = 10;
 *     CachedProperty<MyClass, int> prop = make_cached_property(this, [](MyClass* self){ return self->data * 2; });
 *     // Or, if make_cached_property is a friend or callable is a public static method:
 *     // CachedProperty<MyClass, int> prop{this, &MyClass::calculate_prop};
 * };
 */
template<typename Owner, typename Callable,
         typename T = std::invoke_result_t<Callable, Owner*>>
CachedProperty<Owner, T> make_cached_property(Owner* owner, Callable&& callable) {
    return CachedProperty<Owner, T>(owner, std::forward<Callable>(callable));
}

// Specialization for member function pointers
// This allows a slightly cleaner syntax for member functions:
// make_cached_property(this, &MyStruct::my_method)
template<typename Owner, typename Ret, typename ClassType>
CachedProperty<Owner, Ret> make_cached_property(Owner* owner, Ret(ClassType::*mem_fn)()) {
    static_assert(std::is_base_of_v<ClassType, Owner> || std::is_same_v<ClassType, Owner>,
                  "Member function must belong to Owner type or its base type.");
    return CachedProperty<Owner, Ret>(owner, [mem_fn](Owner* self) {
        return (self->*mem_fn)();
    });
}

// Const member function pointers
template<typename Owner, typename Ret, typename ClassType>
CachedProperty<Owner, Ret> make_cached_property(Owner* owner, Ret(ClassType::*mem_fn)() const) {
    static_assert(std::is_base_of_v<ClassType, Owner> || std::is_same_v<ClassType, Owner>,
                  "Member function must belong to Owner type or its base type.");
    return CachedProperty<Owner, Ret>(owner, [mem_fn](Owner* self) {
        return (self->*mem_fn)();
    });
}

// Member function pointers that take Owner* (less common for this pattern but possible)
// template<typename Owner, typename Ret, typename ClassType>
// CachedProperty<Owner, Ret> make_cached_property(Owner* owner, Ret(ClassType::*mem_fn)(Owner*)) {
//     return CachedProperty<Owner, Ret>(owner, std::bind(mem_fn, std::placeholders::_1, owner));
// }

// Note on thread-safety:
// This implementation is not thread-safe by default. If multiple threads
// access `get()` concurrently before the value is cached, `compute_func_`
// might be called multiple times. The caching itself (assignment to std::optional)
// might also cause a data race.
// For thread-safe computation, `std::call_once` could be used within `get()`.
// Example (conceptual):
// mutable std::once_flag flag_;
// void get() const {
//     std::call_once(flag_, [&]{
//         cached_value_ = compute_func_(owner_ptr_);
//     });
//     return *cached_value_;
// }
// However, this adds overhead and complexity, so it's often left out for
// single-threaded use cases or when external synchronization is applied.
