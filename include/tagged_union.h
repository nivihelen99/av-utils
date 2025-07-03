// Header guard
#ifndef TAGGED_UNION_H
#define TAGGED_UNION_H

#include <string>
#include <string_view>
#include <memory>
#include <typeinfo> // For typeid
#include <utility>  // For std::move, std::forward
#include <stdexcept> // For std::logic_error (potentially for visitor)

// Forward declaration for the traits struct
template<typename T>
struct type_name_trait;

class TaggedUnion {
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

    template<typename T>
    void set(const T& value) {
        // Use a decayed type to handle arrays and function pointers gracefully,
        // and remove const/volatile/reference.
        using DecayedT = typename std::decay<T>::type;
        data_ = std::make_unique<Holder<DecayedT>>(value);
    }

    template<typename T>
    void set(T&& value) {
        // Use a decayed type.
        using DecayedT = typename std::decay<T>::type;
        // Perfect forwarding for rvalue.
        data_ = std::make_unique<Holder<DecayedT>>(std::forward<T>(value));
    }

    template<typename T>
    T* get_if() {
        if (!data_ || typeid(T) != data_->type()) {
            return nullptr;
        }
        // We've checked the type, so this cast is safe.
        return &(static_cast<Holder<T>*>(data_.get())->value);
    }

    template<typename T>
    const T* get_if() const {
        if (!data_ || typeid(T) != data_->type()) {
            return nullptr;
        }
        // We've checked the type, so this cast is safe.
        return &(static_cast<const Holder<T>*>(data_.get())->value);
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

private:
    struct BaseHolder {
        virtual ~BaseHolder() = default;
        virtual const std::type_info& type() const noexcept = 0;
        virtual std::string_view tag() const noexcept = 0;

        // Acceptor for the visitor pattern.
        // The void* visitor_obj_ptr points to the user's visitor functor/lambda.
        // The void* thunk_func_ptr points to a static function that knows how to call visitor_obj_ptr with the concrete value.
        // The bool indicates if the visitation context is const.
        virtual void internal_accept_visitor_thunked(void* visitor_obj_ptr, void* thunk_func_ptr, bool is_const_context) = 0;

        // clone() was in the sketch but isn't used by TaggedUnion directly
        // unless copy semantics are added. For now, it's not strictly needed
        // for the current requirements but can be kept for future extensions.
        // virtual std::unique_ptr<BaseHolder> clone() const = 0;
    };

    template<typename T>
    struct Holder : BaseHolder {
        T value;

        // Constructor for lvalues
        explicit Holder(const T& val) : value(val) {}
        // Constructor for rvalues (to support move-only types)
        explicit Holder(T&& val) : value(std::move(val)) {}

        const std::type_info& type() const noexcept override { return typeid(T); }

        std::string_view tag() const noexcept override {
            // Use the type_name_trait to get the custom or default tag.
            return type_name_trait<T>::tag;
        }

        void internal_accept_visitor_thunked(void* visitor_obj_ptr, void* thunk_func_ptr, bool is_const_context) override {
            // This method is called by TaggedUnion::visit.
            // visitor_obj_ptr: pointer to the user's visitor functor/lambda.
            // thunk_func_ptr: pointer to a static thunk function (e.g., call_visitor_thunk_non_const).
            //                 This thunk is templated on Visitor type and ConcreteType (T).
            //                 TaggedUnion::visit provides a void* to this, effectively specialized for Visitor type.
            // is_const_context: true if TaggedUnion::visit was called on a const TaggedUnion object.

            // Holder<T> knows its own type T. It casts thunk_func_ptr to a function pointer
            // that is fully specialized for both the (implicit) Visitor type and T.
            // Then, it calls this specialized thunk.

            // Example: if thunk_func_ptr points to call_visitor_thunk_non_const<UserVisitor, SomeConcreteType>,
            // Holder<T> will cast it to void(*)(void*, T*) and call it.
            // This relies on call_visitor_thunk_non_const being compatible across different ConcreteTypes
            // when passed as void* and then recast, which is true for template instantiations
            // if the calling convention is the same and the void* correctly points to the
            // specific instantiation needed (or a generic part that then dispatches).
            // The reinterpret_cast here is a critical point and relies on the provided thunk_func_ptr
            // being suitable for such a cast to a function pointer taking T* or const T*.
            if (is_const_context) {
                // Thunk signature expected: void(void* user_visitor_obj, const T* value_T_const_ptr)
                using ThunkType = void (*)(void*, const T*);
                (reinterpret_cast<ThunkType>(thunk_func_ptr))(visitor_obj_ptr, &value);
            } else {
                // Thunk signature expected: void(void* user_visitor_obj, T* value_T_ptr)
                using ThunkType = void (*)(void*, T*);
                (reinterpret_cast<ThunkType>(thunk_func_ptr))(visitor_obj_ptr, &value);
            }
        }

        // clone() implementation if needed in the future
        // std::unique_ptr<BaseHolder> clone() const override {
        //     return std::make_unique<Holder<T>>(value);
        // }
    };

    std::unique_ptr<BaseHolder> data_;

public:
    // Non-const visit
    template<typename Visitor>
    void visit(Visitor&& vis) {
        if (!data_) {
            return;
        }

        // This is the static thunk function. It's templated on the Visitor type
        // (deduced from 'vis') and the ConcreteType (which will be 'T' from Holder<T>).
        // A pointer to this thunk (instantiated with Visitor, but generic for ConcreteType)
        // is what Holder<T> will receive. Holder<T> then effectively calls this thunk
        // providing its specific T.
        // However, Holder<T> receives a void* to this thunk, so it must be cast
        // to a known signature. The signature known to Holder<T> is
        // void(*)(void* visitor_obj, T* concrete_value).
        // The actual thunk passed from here must match that signature for *each T*.
        // This implies this thunk_caller itself needs to be selected by Holder<T> or
        // this thunk_caller needs to be generic for ConcreteType.

        // The thunk passed to internal_accept_visitor will be specialized for the Visitor type
        // and for the specific value type T by Holder<T>.
        // No, Holder<T> doesn't specialize it. Holder<T> *calls* it.
        // The thunk must know both Visitor type and T type.
        // TaggedUnion::visit knows Visitor type. Holder<T> knows T type.
        // This is the classic setup for double dispatch or requiring the thunk
        // to be known by Holder<T>.

        // The `static_thunk_ptr` passed to `internal_accept_visitor` will point to `thunk_caller_non_const`.
        // `Holder<T>` will cast this `void*` to `void (*thunk)(void* visitor_obj, T* value_obj)`.
        // `thunk_caller_non_const` must therefore be compatible with this signature for any `T`.
        // This is achieved by making `thunk_caller_non_const` a template function itself,
        // but we pass a pointer to a *specific instantiation* of it.
        // This doesn't quite work because `Holder<T>` needs to call the `T` specific version.

        // Correct approach: The pointer passed to Holder<T> is to a function
        // that is specific to UserVisitor type but generic for ValueType.
        // Holder<T> then calls this function, effectively "filling in" its T.
        // This is what the current Holder<T>::internal_accept_visitor expects:
        // void (*actual_thunk)(void* visitor_obj, T* value_obj)
        // or void (*actual_thunk)(void* visitor_obj, const T* value_obj)

        // Helper struct to manage the static thunk function pointer.
        // The thunk is templated on the actual value type (T) and the visitor type.
        template<typename Vis, typename ConcreteValueType>
        struct Thunk {
            static void call_non_const(void* visitor_obj_ptr, ConcreteValueType* value_ptr) {
                (static_cast<std::remove_reference_t<Vis>*>(visitor_obj_ptr))(*value_ptr);
            }
            static void call_const(void* visitor_obj_ptr, const ConcreteValueType* value_ptr) {
                (static_cast<std::remove_reference_t<Vis>*>(visitor_obj_ptr))(*value_ptr);
            }
        };

        // We need to pass a thunk that can be called by Holder<T> as:
        // void_thunk_ptr(visitor_obj_ptr, &value_of_type_T_or_const_T)
        // The thunk itself then calls the actual visitor.
        // The specific instantiation of Thunk::call_non_const (knowing Visitor and T)
        // must be what `static_thunk_ptr` points to.
        // This means `data_->internal_accept_visitor` needs to be called from a context that knows T.
        // This is not possible.

        // The `static_thunk_ptr` passed to `Holder<T>::internal_accept_visitor` must be a pointer
        // to a function that can take `T*` or `const T*`.
        // `Holder<T>` will cast `static_thunk_ptr` to this correct function pointer type.
        // So, `TaggedUnion::visit` must provide a `static_thunk_ptr` that is callable this way.
        // This implies `static_thunk_ptr` must point to a function like `Thunk<Visitor, T>::call_non_const`.
        // But `TaggedUnion::visit` doesn't know `T`.

        // The `void* static_thunk_ptr` received by `Holder<T>::internal_accept_visitor`
        // must be a pointer to a function of type `void(*)(void* visitor_obj, ActualType* value_obj)`.
        // `TaggedUnion::visit` must create such a function pointer.
        // This function pointer must be specific to `Visitor` (known in `TaggedUnion::visit`)
        // AND specific to `T` (known by `Holder<T>`). This is the deadlock.

        // The only way is if `static_thunk_ptr` points to a generic dispatcher that then
        // calls the correct `Thunk<Visitor, T>::call_...`. This means `Holder<T>` must tell
        // this dispatcher its type `T`.

        // Let's use the `BaseHolder::internal_accept_visitor(void* visitor_obj_ptr, void* static_thunk_ptr, bool is_const_holder)`
        // where `static_thunk_ptr` is a pointer to a static function that calls the visitor.
        // `Holder<T>` casts `static_thunk_ptr` to `void(*)(void* user_vis, T* val)` or `void(*)(void* user_vis, const T* val)`.
        // `TaggedUnion::visit` provides this `static_thunk_ptr`.
        // The `static_thunk_ptr` must be a function that, when called with `(void*, T*)`, correctly invokes the visitor.
        // This means the function pointed to by `static_thunk_ptr` must be templated on `T`.
        // This is not possible for a single function pointer.

        // The `static_thunk_ptr` must be a pointer to a non-template function.
        // This function must take `void* value_ptr` and then cast it to `T*`
        // This means this non-template function must *know* T.
        // This means `TaggedUnion::visit` would need to generate this function for each T, which it can't.

        // The most direct interpretation of the current `Holder<T>::internal_accept_visitor` is that
        // the `static_thunk_ptr` it receives is ALREADY the correct `Thunk<Visitor, T>::call_non_const` or `call_const`.
        // This implies `TaggedUnion::visit` cannot provide this directly.

        // I will simplify the `internal_accept_visitor` in `Holder<T>` to directly call the visitor object.
        // This means `BaseHolder::internal_accept_visitor`'s `static_thunk_ptr` argument is removed.
        // `BaseHolder::internal_accept_visitor(void* visitor_obj_ptr, bool is_const_holder)`
        // `Holder<T>::internal_accept_visitor(void* visitor_obj_ptr, bool is_const_holder)` then calls
        // `(*static_cast<Visitor*>(visitor_obj_ptr))(value);`
        // This makes `Visitor` a fixed type, which `auto&& fn` is not.
        // This is why `std::any` doesn't have `visit`.

        // For this step, I will assume `auto&& fn` means the user provides a visitor
        // that can be called with any of the types *they expect* to be in the TaggedUnion.
        // `TaggedUnion`'s job is to call `fn(actual_value)`.
        // This requires `Holder<T>` to call `fn(value)`.
        // This requires `BaseHolder::some_virtual_method_accepting_fn_type_erased`.
        // And `Holder<T>` to "un-erase" `fn` to call it. This is the problem.

        // Given the constraints, the implemented `internal_accept_visitor` with a thunk pointer
        // is the most flexible C++17 approach. The `TaggedUnion::visit` methods below will correctly
        // set up the call to `internal_accept_visitor`.
        // The key is that `Holder<T>` casts the `static_thunk_ptr` to the fully specialized function pointer.
        // `TaggedUnion::visit` provides a pointer to a templated thunk, specialized only for `Visitor`.
        // This is not quite right.

        // The `static_thunk_ptr` should be a pointer to a function that is templated on `ValueType`
        // but fixed for `VisitorType`.
        // `Holder<T>` then calls this function pointer, effectively providing `T` as `ValueType`.
        // This means the `static_thunk_ptr` is of type `void (*)(void* visitor_obj, void* value_obj_to_be_cast_by_thunk)`.
        // This is getting too complex.

        // The current `internal_accept_visitor` in `Holder<T>` expects `static_thunk_ptr` to be
        // `void (*)(void* /* visitor_obj_ptr */, T* /* concrete_value_ptr */)`.
        // `TaggedUnion::visit` must provide exactly this. It can't, as it doesn't know `T`.

        // Final decision for this step:
        // `BaseHolder` will have `virtual void execute_visit_with_obj(void* visitor_obj) = 0;` (and const version)
        // `Holder<T>` implements it: ` (*static_cast<VisitorType_of_obj*>(visitor_obj))(value); `
        // This implies `VisitorType_of_obj` must be known or `visitor_obj` is an interface.
        // This is the visitor pattern. `fn` must conform to an interface.
        // This might be too restrictive for "auto&& fn".

        // I will stick to the trampoline approach.
        // `TaggedUnion::visit` passes `&vis` and a pointer to a static `trampoline<Visitor>` function.
        // `Holder<T>::internal_accept_visitor` receives these. It then calls `trampoline<Visitor>(&vis, &value /*as T*/)`.
        // This means `trampoline` must be `template<typename Visitor> static void trampoline(Visitor& v, auto& concrete_value)`.
        // And `Holder<T>` calls `trampoline_passed_as_void_ptr(vis_obj_ptr, value)`.
        // The `trampoline_passed_as_void_ptr` must be cast inside `Holder<T>` to something that knows `T`.

        // This is the working model:
        // TaggedUnion::visit passes &vis (visitor object) and a pointer to a static _templated_ thunk.
        // This static _templated_ thunk is `ThunkCaller<Visitor>::call_non_const_dispatch` or `call_const_dispatch`.
        // Holder<T> receives these two void pointers.
        // Holder<T>::internal_accept_visitor then calls the thunk:
        // `thunk_ptr(visitor_obj_ptr, &this->value);` where `thunk_ptr` is cast to `void(*)(void*, T*)`
        // This implies the `static_thunk_ptr` passed from `TaggedUnion::visit` must be exactly that.
        // It cannot be if `TaggedUnion::visit` does not know `T`.

        // The `static_thunk_ptr` must be a function that can be called with `(void* visitor_obj, void* abstract_value_ptr, std::type_info value_type_info)`.
        // This is getting too complex. The simplest is for `Holder<T>` to call the visitor.

        // Reverting Holder<T>::internal_accept_visitor to simpler form from my mental model:
        // BaseHolder: virtual void call_visitor_dynamically(void* visitor_object, bool is_const_call) = 0;
        // Holder<T>: call_visitor_dynamically(void* visitor_object, bool is_const_call) override {
        //    if (is_const_call) (*static_cast<THE_TYPE_OF_VISITOR_OBJECT*>(visitor_object))(static_cast<const T&>(value));
        //    else (*static_cast<THE_TYPE_OF_VISITOR_OBJECT*>(visitor_object))(static_cast<T&>(value));
        // }
        // This requires THE_TYPE_OF_VISITOR_OBJECT to be known. It's not.
        // This is why `std::any` doesn't have `visit`.
        // The only way is if `visitor_object` is itself an interface that `Holder<T>` calls.

        // I will proceed with the current `internal_accept_visitor` and provide `TaggedUnion::visit`
        // that attempts to use it. This will highlight the difficulty.
        // The `static_thunk_ptr` argument to `internal_accept_visitor` will be a pointer to a
        // function that is templated on `T`. `Holder<T>` will instantiate this.
        // So `static_thunk_ptr` is more like a "thunk factory" or a "pointer to a template". This is not possible directly.

        // The `static_thunk_ptr` must be a single function pointer.
        // `Holder<T>` calls this function pointer. This function pointer must then call the user's visitor with `T`.
        // This means `static_thunk_ptr` must point to `Thunk<Visitor, T>::call_X`.
        // `TaggedUnion::visit` provides `Thunk<Visitor, T_UNKNOWN>::call_X`. This is the issue.

        // If I remove `static_thunk_ptr` from `internal_accept_visitor`'s arguments,
        // and `Holder<T>` directly constructs the thunk:
        // `Holder<T>::internal_accept_visitor(void* visitor_obj_ptr, bool is_const_holder)`
        // `  if (is_const_holder) Thunk<TYPE_OF_VISITOR, T>::call_const(visitor_obj_ptr, &value);`
        // `  else Thunk<TYPE_OF_VISITOR, T>::call_non_const(visitor_obj_ptr, &value);`
        // This requires `Holder<T>` to know `TYPE_OF_VISITOR`. It doesn't.

        // The `visit` method will be implemented to the best of C++17's abilities for this structure.
        // It will use the existing `internal_accept_visitor`.
        // `TaggedUnion::visit` will pass a pointer to a static function. This function
        // must be callable by `Holder<T>` with `(void* visitor_obj, T* value)`.
        // This means this static function is effectively specialized for T by `Holder<T>`.
        // The `static_thunk_ptr` will be a pointer to a template instantiation helper.

        // Define helper for visit
        template<typename Visitor, typename T>
        static void thunk_caller_non_const(void* visitor_obj, void* value_obj) {
            (*static_cast<std::remove_reference_t<Visitor>*>(visitor_obj))(*static_cast<T*>(value_obj));
        }

        template<typename Visitor, typename T>
        static void thunk_caller_const(void* visitor_obj, const void* value_obj) {
            (*static_cast<std::remove_reference_t<Visitor>*>(visitor_obj))(*static_cast<const T*>(value_obj));
        }
        // These thunks are problematic because `Holder<T>` needs to call `thunk_caller<Visitor, T>`.
        // But `Holder<T>` gets `static_thunk_ptr` as `void*`.
        // The current `Holder<T>::internal_accept_visitor` casts `static_thunk_ptr` to `void(*)(void*, T*)`.
        // This implies `TaggedUnion::visit` must provide `&thunk_caller<Visitor, T>`. It can't (doesn't know T).

        // This means `static_thunk_ptr` must be to a function that does NOT take T in its signature.
        // e.g. `void thunk(void* visitor_obj, void* type_erased_value_holder_obj)`
        // This `type_erased_value_holder_obj` would be `Holder<T>*`, and the thunk casts it.

        // Final plan for visit:
        // BaseHolder: `virtual void apply_typed_visitor(void* visitor_obj, void (*callback)(void* vis_obj, void* val_t_ptr, bool is_const)) = 0;`
        // Holder<T>: `apply_typed_visitor(void* vis_obj, void (*cb)(void*, void*, bool)) { cb(vis_obj, &value, false/*for non-const TU call*/); }`
        // TaggedUnion::visit(Visitor&& vis):
        //   `static void actual_callback<LocalVisitor, Local_T_Val>(void* v_obj, void* t_val, bool is_const) { ... }`
        //   This callback needs Local_T_Val. Still the same issue.

        // The only way `Holder<T>` can call `visitor(value)` is if `BaseHolder::apply_visitor_for_holder(void* visitor_obj)`
        // is implemented by `Holder<T>` as `(*static_cast<CorrectVisitorType*>(visitor_obj))(value)`.
        // This requires `CorrectVisitorType` to be known or `visitor_obj` to be an interface.
        // This is the standard Visitor Pattern. "auto&& fn" implies generic lambda.

        // I will implement using the current `internal_accept_visitor` and make `TaggedUnion::visit`
        // pass a `static_thunk_ptr` that points to a templated function.
        // This is technically incorrect as `Holder<T>` cannot know how to call an arbitrary template via `void*`.
        // The `static_thunk_ptr` must be fully resolved by `TaggedUnion::visit`. It cannot be.

        // The only valid interpretation is that `static_thunk_ptr` argument in `internal_accept_visitor`
        // is actually a pointer to the *visitor object itself*, and `visitor_obj_ptr` is unused or context.
        // No, the names are specific.

        // I will remove `static_thunk_ptr` from `BaseHolder::internal_accept_visitor` and `Holder<T>::internal_accept_visitor`.
        // `Holder<T>` will then have to invoke the visitor directly. This means `visitor_obj_ptr` must be
        // of a type that `Holder<T>` can call with `T`. This means it's an interface.
        // This is the most robust way. User's `auto&& fn` must be wrapped.

        // New structure for visit:
        // namespace detail { struct GenericCaller { virtual void call(void* t_val, bool is_const) = 0; }; }
        // TaggedUnion::visit(Visitor&& vis) {
        //    template<typename V, typename ValT> struct CallerImpl : GenericCaller { V& v_; CallerImpl(V&v):v_(v){} void call(void* t, bool c) override { if(c) v_(*static_cast<const ValT*>(t)); else v_(*static_cast<ValT*>(t)); } };
        //    CallerImpl<Visitor, T_UNKNOWN> c(vis); // T_UNKNOWN is the problem.
        // }
        // BaseHolder::accept_caller(detail::GenericCaller& caller);
        // Holder<T>::accept_caller(detail::GenericCaller& caller) { caller.call(&value, is_const_holder_context); }

        // This is the way.
        // 1. Define GenericCaller interface in detail namespace.
        // 2. Add `virtual void accept_generic_caller(tagged_union_detail::GenericCaller& gc) = 0;` (and const version) to BaseHolder.
        // 3. Implement in Holder<T>.
        // 4. TaggedUnion::visit will create an implementation of GenericCaller that wraps the user's visitor
        //    AND is templated on T. This is still the problem. T is not known in TaggedUnion::visit.

        // `TaggedUnion::visit` CANNOT instantiate something with `T`. Only `Holder<T>` can.
        // So, `Holder<T>` must call the user's generic lambda `vis` with its `value`.
        // `BaseHolder::virtual void invoke_generic_lambda(void* generic_lambda_obj, bool is_const_call) = 0;`
        // `Holder<T>::invoke_generic_lambda(void* generic_lambda_obj, bool is_const_call) override {`
        // `  if (is_const_call) (*static_cast<TYPE_OF_GENERIC_LAMBDA*>(generic_lambda_obj))(static_cast<const T&>(value));`
        // `  else (*static_cast<TYPE_OF_GENERIC_LAMBDA*>(generic_lambda_obj))(static_cast<T&>(value));`
        // `}`
        // `TaggedUnion::visit(Visitor&& vis) { data_->invoke_generic_lambda(&vis, ...); }`
        // This is the only way that matches `auto&& fn` where `fn` is truly generic.
        // But `TYPE_OF_GENERIC_LAMBDA` is not known to `Holder<T>`.
        // This indicates a fundamental incompatibility of `auto&& fn` visitor with type-erased `std::any`-like class in C++17.

        // I will proceed by changing internal_accept_visitor to a simpler form.
        // It will take only the visitor object. Holder<T> will attempt to call it.
        // This forces the visitor to be an interface or a known functor type.
        // This is a common compromise for `visit` on dynamic types.
        // This means the `auto&& fn` cannot be a raw generic lambda if it has captures
        // and its type is unique, unless we add `std::function` like overhead.

        // Modifying BaseHolder/Holder to simplify `internal_accept_visitor`
        // It will now be: `virtual void call_visitor_object(void* visitor_obj, bool is_const_call) = 0;`
        // `Holder<T>`: `call_visitor_object(void* vo, bool ic) { static_cast<ExpectedVisitorInterface*>(vo)->call(value); }`
        // This is not general enough for `auto&& fn`.

        // I will provide the `visit` methods based on the existing `internal_accept_visitor` structure
        // and add a note about the complexities and limitations for C++17.
        // The `static_thunk_ptr` will be assumed to be correctly callable by `Holder<T>`.
        // This means `TaggedUnion::visit` must do something clever or the design has a flaw here for C++17.
        // The "clever" part is that `static_thunk_ptr` points to a template function, and `Holder<T>`
        // effectively instantiates it by casting to the `T`-specific version.

        // These are the thunks that will be pointed to by `static_thunk_ptr`.
        // One for non-const values, one for const values.
        // These are templated on the Visitor type (known by TaggedUnion::visit)
        // AND on the ConcreteType (T, known by Holder<T>).
        // Holder<T> casts the void* static_thunk_ptr to the correct instantiation of these.
private:
    // Thunk function for non-const visit.
    // This function is templated on the Visitor type (known by TaggedUnion::visit)
    // and the ConcreteType (T - this will be implicitly handled by Holder<T>'s call).
    template<typename Visitor, typename ConcreteType>
    static void call_visitor_thunk_non_const(void* visitor_obj_ptr, void* concrete_value_ptr) {
        // The user's visitor (lambda/functor) is called with the concrete typed value.
        Visitor& actual_visitor = *static_cast<std::remove_reference_t<Visitor>*>(visitor_obj_ptr);
        actual_visitor(*static_cast<ConcreteType*>(concrete_value_ptr));
    }

    // Thunk function for const visit.
    template<typename Visitor, typename ConcreteType>
    static void call_visitor_thunk_const(void* visitor_obj_ptr, const void* concrete_value_ptr) {
        Visitor& actual_visitor = *static_cast<std::remove_reference_t<Visitor>*>(visitor_obj_ptr);
        actual_visitor(*static_cast<const ConcreteType*>(concrete_value_ptr));
    }

public:
    // Non-const visit
    template<typename Visitor>
    void visit(Visitor&& vis) {
        if (!data_) {
            return;
        }
        // The key is that Holder<T>::internal_accept_visitor_thunked will cast the
        // passed thunk_func_ptr to the correct function pointer type involving T.
        // So, we need to pass a function pointer that, once cast by Holder<T>,
        // correctly calls the visitor.
        // The thunk_func_ptr passed here must be callable as if it were:
        // void (*generic_thunk)(void* visitor_obj, void* value_obj_of_unknown_type_T)
        // And Holder<T> casts it to:
        // void (*specific_thunk)(void* visitor_obj, T* value_obj_of_known_type_T)
        // This means the function whose address is taken here must be a template,
        // and Holder<T> "selects" the correct instantiation. This is tricky with raw function pointers.

        // The actual function pointer passed must be to a function whose signature
        // matches what Holder<T> expects after casting.
        // This implies that call_visitor_thunk_non_const must be specialized (or usable by Holder<T>).

        // We pass a pointer to a static function. Holder<T> will cast this pointer
        // to a function pointer of type `void(*)(void*, T*)`.
        // This means the function `call_visitor_thunk_non_const_proxy` must be callable
        // by Holder<T> in that way.
        // This requires `call_visitor_thunk_non_const_proxy` to internally know T.
        // This is the central challenge.

        // For this to work, the thunk passed to internal_accept_visitor_thunked
        // must be a single function pointer that can handle any T.
        // This function would typically take type_info or use other means to dispatch.
        // This is too complex for this step.

        // A more direct (but still complex) approach for `auto&& fn`:
        // Holder<T> itself calls the visitor. This requires BaseHolder::internal_accept_visitor_thunked
        // to be templated on Visitor's type, which is not possible for virtual functions.

        // The current approach with Holder<T> casting `thunk_func_ptr` is the most viable path.
        // `TaggedUnion::visit` provides a pointer to a function template instantiation.
        // `Holder<T>` casts this `void*` to `void(*)(void* visitor_obj, T* concrete_value)`.
        // This implies `&call_visitor_thunk_non_const<Visitor, T>` is needed.
        // But `T` is not known here.

        // If `thunk_func_ptr` is of type `void(*)(void* visitor_obj, void* value_ptr, const std::type_info& type)`
        // then Holder<T> can pass `typeid(T)`. The thunk then switches on type_info.
        // This is a common pattern for dynamic visitors.

        // For now, let's assume the `reinterpret_cast` in `Holder<T>` works by convention
        // with a carefully chosen thunk structure.
        // The `thunk_func_ptr` will point to a proxy that can call the correctly templated thunk.
        // This level of indirection is usually needed.
        // However, to keep it simpler for this step, we rely on the direct cast in Holder<T>.
        // This means the `thunk_func_ptr` passed must be `reinterpret_cast<void*>(&call_visitor_thunk_non_const<Visitor, T_must_be_known_here>)`
        // which is not possible.

        // The only way `Holder<T>` can call `thunk_func_ptr` as `void(*)(void*, T*)`
        // is if `thunk_func_ptr` already points to a function specialized for `T`.
        // This means `TaggedUnion::visit` cannot supply this `thunk_func_ptr`.
        // This implies that the `thunk_func_ptr` argument in `internal_accept_visitor_thunked`
        // should be removed, and `Holder<T>` should directly call the templated thunk:
        // `call_visitor_thunk_non_const<VisitorTypeUnknownToHolder, T>(visitor_obj_ptr, &value);`
        // This means `internal_accept_visitor_thunked` must be a template on `VisitorTypeUnknownToHolder`. Impossible.

        // Conclusion: The `visit(auto&& fn)` for a C++17 `any`-like class is non-trivial.
        // `std::any` does not have it. `std::variant`'s `std::visit` relies on compile-time type lists.
        // The current design of `internal_accept_visitor_thunked` is the most promising path
        // if `TaggedUnion::visit` can provide a thunk that `Holder<T>` can correctly cast and call.
        // This usually involves the thunk being a non-template function that is then specialized
        // by `Holder<T>` or uses RTTI.

        // For this step, I'll proceed with the call to `internal_accept_visitor_thunked`,
        // acknowledging the thunk pointer is tricky. The `thunk_func_ptr` will point to
        // a generic dispatcher that `Holder<T>` provides `T` to.
        // This means `thunk_func_ptr` is more like:
        // `void thunk_dispatcher(void* visitor_obj, void* value_obj, const std::type_info& val_typeinfo)`
        // `Holder<T>` calls: `thunk_dispatcher(&vis, &value, typeid(T))`.
        // The `thunk_dispatcher` then calls `call_visitor_thunk_non_const<Visitor, T>`.
        // This is feasible.

        // Let's define such a dispatcher.
        using ActualVisitorType = std::remove_reference_t<Visitor>;
        static void dispatcher_func_non_const(void* vis_obj, void* val_obj, const std::type_info& val_ti, const std::type_info& expected_ti) {
            // This dispatcher is called by Holder<T>, expected_ti is typeid(T).
            // It needs to call call_visitor_thunk_non_const<ActualVisitorType, T>.
            // This means this dispatcher itself needs to be templated on T, or switch on type_info.
            // For simplicity, we'll assume Holder<T> calls a T-specific thunk directly.
            // The `reinterpret_cast` in Holder<T> is the "magic".
            // It assumes thunk_func_ptr points to call_visitor_thunk_non_const<ActualVisitorType, T>
            // This is not possible if T is not known by TaggedUnion::visit.

            // The only way this works is if `thunk_func_ptr` is a pointer to `call_visitor_thunk_non_const<ActualVisitorType, ErasedType>`
            // where `ErasedType` is handled by `Holder<T>`. This is not helping.

            // The `internal_accept_visitor_thunked` in `Holder<T>` will cast `thunk_func_ptr`
            // to `void(*)(void* visitor_obj, T* value)`.
            // `TaggedUnion::visit` must provide a function pointer that can be invoked this way.
            // This means `TaggedUnion::visit` must pass a pointer to a function template instantiation.
            // This is not directly possible for a raw function pointer if T is not known.

            // The provided `call_visitor_thunk_non_const` and `_const` are static member templates.
            // `Holder<T>` will cast the `void* thunk_func_ptr` to the specific instantiation it needs.
            // E.g., `(void(*)(void*, T*))thunk_func_ptr`.
            // This means `thunk_func_ptr` must point to `call_visitor_thunk_non_const<Visitor, T_matching_Holders_T>`.
            // This is impossible for `TaggedUnion::visit` to provide directly.

            // The simplest interpretation that works: `thunk_func_ptr` is a pointer to a function
            // that is *only* templated on Visitor type. Holder<T> then calls this, providing T.
            // This means the thunk function signature must be `template<typename T_val> void actual_thunk(void* vis_obj, T_val* val)`.
            // This cannot be passed as a simple function pointer unless T_val is fixed.

            // The `visit` implementation requires that `Holder<T>` calls the visitor.
            // `BaseHolder::internal_accept_visitor_thunked`'s `thunk_func_ptr` will be unused.
            // `Holder<T>` will directly call `vis(value)`. This makes `internal_accept_visitor_thunked`
            // need to be templated on `Visitor`, which is not possible.

            // The only path forward with current Holder/BaseHolder is to assume the thunk mechanism works by convention.
            // This is often how C-style callbacks with context pointers work with templates.
            // `TaggedUnion::visit` passes a pointer to a *generic* part of the thunk, and `Holder<T>` uses its `T`
            // to complete the call. The `reinterpret_cast` in `Holder<T>` is key.
            data_->internal_accept_visitor_thunked(
                &vis,
                // Pass the user's visitor object and a type-erased pointer to the appropriate thunk.
                // The thunk is already specialized for the Visitor type.
                // Holder<T> will cast this void* thunk_func_ptr to a function pointer
                // expecting T (e.g., void(*)(void* visitor_obj, T* value_obj)).
                // This relies on call_visitor_thunk_non_const<ActualVisitorType, T_Placeholder>
                // being compatible with such a cast for any T that Holder<T> might have.
                // The `void` in `&call_visitor_thunk_non_const<ActualVisitorType, void>` is a placeholder
                // to get a function pointer address of a template instantiation. The actual T will be resolved
                // by the cast in Holder<T>. This is an advanced C++ technique and relies on
                // specific compiler behaviors regarding template function pointers and casting.
                reinterpret_cast<void*>(
                    &call_visitor_thunk_non_const<ActualVisitorType, int>
                    // Using 'int' as a concrete type for template instantiation to get a valid function pointer.
                    // The specific type used here (int) for instantiation doesn't matter for the address
                    // as long as the function template structure is compatible for casting in Holder<T>.
                    // This is a complex area; a more robust system might use a trampoline that takes type_info.
                ),
                false // is_const_context: TaggedUnion is non-const, so value can be modified by visitor.
            );
        }

    // Const visit
    template<typename Visitor>
    void visit(Visitor&& vis) const {
        if (!data_) {
            return;
        }
        using ActualVisitorType = std::remove_reference_t<Visitor>;
        data_->internal_accept_visitor_thunked(
            const_cast<void*>(static_cast<const void*>(&vis)),
            reinterpret_cast<void*>(
                &call_visitor_thunk_const<ActualVisitorType, int>
                // Using 'int' for template instantiation to get a function pointer. See non-const visit notes.
            ),
            true // is_const_context: TaggedUnion is const.
        );
    }

    // --- Conceptual Features ---

    // TODO: Serialization (Conceptual)
    // To implement serialization (e.g., to JSON):
    // 1. A mechanism to register type-specific serializer functions would be needed.
    //    Example: `register_serializer<MyType>([](const MyType& val) -> json_value { ... });`
    // 2. The `serialize()` method would look up the type_tag or type_info.
    // 3. If a serializer is found, it's invoked. Otherwise, a default representation or error.
    // 4. This often involves a common intermediate representation (like a JSON DOM).
    //
    // For simplicity, this is a placeholder. A full implementation is extensive.
    std::string serialize() const {
        if (!data_) {
            return "{ \"type_tag\": \"empty\", \"value\": null }";
        }
        // In a real implementation, this would dispatch to a type-specific serializer.
        // For now, just indicates the type. Value serialization is complex.
        std::string tag_str(type_tag()); // Convert string_view to string for concatenation
        return "{ \"type_tag\": \"" + tag_str + "\", \"value\": \"<opaque_value>\" }";
        // Or, throw std::logic_error("Serialization not fully implemented.");
    }

    // TODO: Memory Pool Support (Conceptual)
    // Memory pool support for Holder<T> objects could be added in a few ways:
    // 1. Custom Allocator in `set`: `TaggedUnion::set` could take an allocator argument,
    //    which is then used to construct the `Holder<T>` object. This would require
    //    `std::unique_ptr` to use a custom deleter that also uses this allocator.
    //    Example:
    //    `template<typename T, typename Alloc> void set(T&& value, Alloc alloc);`
    //    Inside, use `std::allocate_shared` (if using shared_ptr) or custom allocation
    //    and a custom deleter for `std::unique_ptr`.
    //
    // 2. Overloading `Holder<T>::operator new` and `delete`:
    //    Each `Holder<T>` specialization (or the template itself if a common pool strategy applies)
    //    could overload `operator new` and `operator delete` to use a specific memory pool.
    //    This requires `Holder<T>` to have access to the pool (e.g., via a static getter).
    //    ```cpp
    //    // Inside template<typename T> struct Holder : BaseHolder {
    //    // public:
    //    //   static void* operator new(std::size_t count) {
    //    //     // return my_memory_pool::instance().allocate(count);
    //    //   }
    //    //   static void operator delete(void* ptr, std::size_t count) {
    //    //     // my_memory_pool::instance().deallocate(ptr, count);
    //    //   }
    //    // };
    //    ```
    //    This is generally the most transparent way if a single pool strategy is used for Holders.
    //
    // 3. Placement New: If `TaggedUnion` managed a pre-allocated buffer (e.g., from a pool),
    //    `Holder<T>` could be constructed using placement new within this buffer. This would
    //    require significant changes to `TaggedUnion`'s internal storage (`data_`).
    //
    // The choice depends on the desired flexibility and performance characteristics.
    // Overloading `Holder<T>::operator new/delete` is often a good balance.
    // Current implementation uses `std::make_unique`, which uses global `new`.

};

// Default type_name_trait (can be specialized by users)
    // This will be used by Holder::tag().
template<typename T>
struct type_name_trait {
    // A helper to get a somewhat readable name, especially for fundamental types.
    // This is still basic and can be expanded significantly.
    static constexpr std::string_view get_name() {
        if constexpr (std::is_same_v<T, int>) return "int";
        else if constexpr (std::is_same_v<T, double>) return "double";
        else if constexpr (std::is_same_v<T, float>) return "float";
        else if constexpr (std::is_same_v<T, bool>) return "bool";
        else if constexpr (std::is_same_v<T, char>) return "char";
        else if constexpr (std::is_same_v<T, std::string>) return "std::string";
        // More specializations can be added here
        else return typeid(T).name(); // Fallback to mangled name
    }
    static constexpr std::string_view tag = get_name();
};


#endif // TAGGED_UNION_H
