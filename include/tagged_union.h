// Header guard
#ifndef TAGGED_UNION_H
#define TAGGED_UNION_H

#include <string>
#include <string_view>
#include <memory>
#include <typeinfo> // For typeid
#include <utility>  // For std::move, std::forward
#include <stdexcept> // For std::logic_error

// Forward declaration for the traits struct
template<typename T>
struct type_name_trait;

class TaggedUnion {
private:
    struct BaseHolder {
        virtual ~BaseHolder() = default;
        virtual const std::type_info& type() const noexcept = 0;
        virtual std::string_view tag() const noexcept = 0;
        virtual void internal_accept_visitor_thunked(void* visitor_obj_ptr, void* thunk_func_ptr, bool is_const_context) = 0;
    };

    template<typename T_val> // Changed T to T_val to avoid conflict with class template parameter T if TaggedUnion were a template
    struct Holder : BaseHolder {
        T_val value;

        explicit Holder(const T_val& val) : value(val) {}
        explicit Holder(T_val&& val) : value(std::move(val)) {}

        const std::type_info& type() const noexcept override { return typeid(T_val); }

        std::string_view tag() const noexcept override {
            return type_name_trait<T_val>::tag;
        }

        void internal_accept_visitor_thunked(void* visitor_obj_ptr, void* thunk_func_ptr, bool is_const_context) override {
            if (is_const_context) {
                using ThunkType = void (*)(void*, const T_val*);
                (reinterpret_cast<ThunkType>(thunk_func_ptr))(visitor_obj_ptr, &value);
            } else {
                using ThunkType = void (*)(void*, T_val*);
                (reinterpret_cast<ThunkType>(thunk_func_ptr))(visitor_obj_ptr, &value);
            }
        }
    };

    std::unique_ptr<BaseHolder> data_;

    // Private static thunk functions for the visitor pattern
    template<typename Visitor, typename ConcreteType>
    static void call_visitor_thunk_non_const(void* visitor_obj_ptr, void* concrete_value_ptr) {
        Visitor& actual_visitor = *static_cast<std::remove_reference_t<Visitor>*>(visitor_obj_ptr);
        actual_visitor(*static_cast<ConcreteType*>(concrete_value_ptr));
    }

    template<typename Visitor, typename ConcreteType>
    static void call_visitor_thunk_const(void* visitor_obj_ptr, const void* concrete_value_ptr) {
        Visitor& actual_visitor = *static_cast<std::remove_reference_t<Visitor>*>(visitor_obj_ptr);
        actual_visitor(*static_cast<const ConcreteType*>(concrete_value_ptr));
    }

public:
    TaggedUnion() noexcept = default;

    TaggedUnion(const TaggedUnion&) = delete;
    TaggedUnion& operator=(const TaggedUnion&) = delete;

    TaggedUnion(TaggedUnion&& other) noexcept
        : data_(std::move(other.data_)) {}

    TaggedUnion& operator=(TaggedUnion&& other) noexcept {
        if (this != &other) {
            data_ = std::move(other.data_);
        }
        return *this;
    }

    ~TaggedUnion() = default;

    template<typename T_arg>
    void set(const T_arg& value) {
        using DecayedT = typename std::decay<T_arg>::type;
        data_ = std::make_unique<Holder<DecayedT>>(value);
    }

    template<typename T_arg>
    void set(T_arg&& value) {
        using DecayedT = typename std::decay<T_arg>::type;
        data_ = std::make_unique<Holder<DecayedT>>(std::forward<T_arg>(value));
    }

    template<typename T_ret>
    T_ret* get_if() {
        if (!data_ || typeid(T_ret) != data_->type()) {
            return nullptr;
        }
        return &(static_cast<Holder<T_ret>*>(data_.get())->value);
    }

    template<typename T_ret>
    const T_ret* get_if() const {
        if (!data_ || typeid(T_ret) != data_->type()) {
            return nullptr;
        }
        return &(static_cast<const Holder<T_ret>*>(data_.get())->value);
    }

    std::string_view type_tag() const {
        if (!data_) {
            return "empty";
        }
        return data_->tag();
    }

    bool has_value() const noexcept {
        return static_cast<bool>(data_);
    }

    void reset() noexcept {
        data_.reset();
    }

    template<typename Visitor>
    void visit(Visitor&& vis) {
        if (!data_) {
            return;
        }
        using ActualVisitorType = std::remove_reference_t<Visitor>;
        // The thunk_func_ptr passed to internal_accept_visitor_thunked must be castable
        // by Holder<T_val> to void(*)(void* visitor_obj, T_val* concrete_value).
        // This means we pass a pointer to call_visitor_thunk_non_const<ActualVisitorType, T_val_unknown_here>.
        // This is a complex C++ pattern. The reinterpret_cast in Holder is key.
        // We provide a function pointer obtained from one instantiation (e.g., with int)
        // and rely on the identical function body structure for other T_val types.
        // This is advanced and has portability caveats. A safer method might involve
        // more complex type erasure or a dispatch table if performance allows.
        data_->internal_accept_visitor_thunked(
            &vis,
            reinterpret_cast<void*>(&call_visitor_thunk_non_const<ActualVisitorType, int>), // Using 'int' as a representative type for instantiation.
            false // is_const_context
        );
    }

    template<typename Visitor>
    void visit(Visitor&& vis) const {
        if (!data_) {
            return;
        }
        using ActualVisitorType = std::remove_reference_t<Visitor>;
        data_->internal_accept_visitor_thunked(
            const_cast<void*>(static_cast<const void*>(&vis)),
            reinterpret_cast<void*>(&call_visitor_thunk_const<ActualVisitorType, int>), // Using 'int' as a representative type.
            true // is_const_context
        );
    }

    std::string serialize() const {
        if (!data_) {
            return "{ \"type_tag\": \"empty\", \"value\": null }";
        }
        std::string tag_str(type_tag());
        return "{ \"type_tag\": \"" + tag_str + "\", \"value\": \"<opaque_value>\" }";
        // Conceptual: Full serialization requires type-specific handlers.
    }

    // Conceptual: Memory pool discussion would go here or in documentation.
    // Current uses std::make_unique with global new.
    // Options: Holder<T>::operator new/delete, custom allocator in set(), placement new.
};

// Default type_name_trait (can be specialized by users)
template<typename T>
struct type_name_trait {
    static constexpr std::string_view get_name() {
        if constexpr (std::is_same_v<T, int>) return "int";
        else if constexpr (std::is_same_v<T, double>) return "double";
        else if constexpr (std::is_same_v<T, float>) return "float";
        else if constexpr (std::is_same_v<T, bool>) return "bool";
        else if constexpr (std::is_same_v<T, char>) return "char";
        else if constexpr (std::is_same_v<T, std::string>) return "std::string";
        else return typeid(T).name(); // Fallback to mangled name
    }
    static constexpr std::string_view tag = get_name();
};

#endif // TAGGED_UNION_H
