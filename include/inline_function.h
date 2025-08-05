#pragma once

#include <type_traits>
#include <utility>
#include <cassert>
#include <memory>
#include <cstring>
#include <stdexcept>
#include <functional>

// inline_function: simplified small-buffer-optimized callable wrapper
template <typename Signature, size_t InlineSize = 3 * sizeof(void*)>
class inline_function; // primary

// specialization for function types
template <typename R, typename... Args, size_t InlineSize>
class inline_function<R(Args...), InlineSize> {
private:
    // function pointer types for the vtable
    using invoke_fn_t = R(*)(void*, Args&&...);
    using destroy_fn_t = void(*)(void*);
    using move_fn_t = void(*)(void* dest, void* src);

    struct vtable_t {
        invoke_fn_t invoke;
        destroy_fn_t destroy;
        move_fn_t move;
    };

    alignas(std::max_align_t) unsigned char buffer[InlineSize];
    void* object_ptr = nullptr;
    const vtable_t* vptr = nullptr;

    template <typename F>
    static R invoke_fn(void* obj, Args&&... args) {
        return (*static_cast<F*>(obj))(std::forward<Args>(args)...);
    }

    template <typename F>
    static void destroy_fn(void* obj) noexcept {
        static_cast<F*>(obj)->~F();
    }

    template <typename F>
    static void move_fn(void* dest, void* src) noexcept {
        new (dest) F(std::move(*static_cast<F*>(src)));
        destroy_fn<F>(src);
    }

    template <typename F>
    static const vtable_t* get_vtable() {
        static const vtable_t vt = {
            &invoke_fn<F>,
            &destroy_fn<F>,
            &move_fn<F>
        };
        return &vt;
    }

    void* storage() noexcept { return static_cast<void*>(buffer); }

public:
    inline_function() noexcept = default;

    inline_function(std::nullptr_t) noexcept {}

    // construct from callable
    template <
        typename F,
        typename = std::enable_if_t<!std::is_same_v<std::decay_t<F>, inline_function>>,
        typename = std::enable_if_t<std::is_invocable_r_v<R, F&, Args...>>
    >
    inline_function(F&& f) {
        using Fun = std::decay_t<F>;
        static_assert(sizeof(Fun) <= InlineSize, "Callable too large for inline buffer");
        static_assert(std::is_nothrow_move_constructible_v<Fun>, "Functor must be nothrow-movable");

        object_ptr = storage();
        new (object_ptr) Fun(std::forward<F>(f));
        vptr = get_vtable<Fun>();
    }

    // move constructor
    inline_function(inline_function&& other) noexcept {
        if (this == &other) {
            return;
        }
        if (other.vptr) {
            object_ptr = storage();
            other.vptr->move(object_ptr, other.object_ptr);
            vptr = other.vptr;
            other.vptr = nullptr;
            other.object_ptr = nullptr;
        }
    }

    // move assignment
    inline_function& operator=(inline_function&& other) noexcept {
        if (this != &other) {
            reset();
            if (other.vptr) {
                object_ptr = storage();
                other.vptr->move(object_ptr, other.object_ptr);
                vptr = other.vptr;
                other.vptr = nullptr;
                other.object_ptr = nullptr;
            }
        }
        return *this;
    }

    inline_function& operator=(std::nullptr_t) noexcept {
        reset();
        return *this;
    }

    // disable copy
    inline_function(const inline_function&) = delete;
    inline_function& operator=(const inline_function&) = delete;

    ~inline_function() {
        reset();
    }

    void reset() noexcept {
        if (vptr) {
            vptr->destroy(object_ptr);
            vptr = nullptr;
            object_ptr = nullptr;
        }
    }

    explicit operator bool() const noexcept {
        return vptr != nullptr;
    }

    R operator()(Args... args) {
        if (!vptr) {
            throw std::bad_function_call();
        }
        return vptr->invoke(object_ptr, std::forward<Args>(args)...);
    }
};
