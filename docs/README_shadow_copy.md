# `ShadowCopy<T>`: Mutable View with Change Tracking

## Purpose

`ShadowCopy<T>` is a C++17 header-only utility that provides a "copy-on-write" proxy to an object of type `T`. It allows temporary modifications to an object while keeping the original intact. This is useful in scenarios where changes might be conditionally committed or discarded, or when you need to compare a modified state against a baseline.

## Key Features

*   **Lazy Copying**: A separate "shadow" copy of the object is only created when modifications are attempted via the `get()` method.
*   **Change Tracking**: The `modified()` method indicates if `get()` has been called or if the shadow value (if it exists) differs from the original.
*   **Access to States**: Provides access to the immutable `original()` state and the `current()` (potentially modified) state.
*   **State Management**:
    *   `commit()`: Applies the changes from the shadow copy to the original object.
    *   `reset()`: Discards any changes and reverts to the original state.
    *   `take()`: Moves the shadow value out, leaving the `ShadowCopy` with its original value.

## Use Cases

*   **Configuration Editors**: Allow users to change settings and then decide to apply or cancel.
*   **Transactional Updates**: Bundle a set of changes and commit them atomically or roll back.
*   **UI State Management**: Track if a data model backing a UI element has been altered by user input.
*   **Diff/Patch Systems**: Prepare a modified version of data and then compare it to the original to generate a diff.
*   **Testing & Mocking**: Temporarily alter the state of an object for a test without affecting a global or shared instance.

## API Sketch

```cpp
template <typename T>
class ShadowCopy {
public:
    // Construction
    ShadowCopy(const T& value);          // Initialize with an existing value (copied)
    ShadowCopy(T&& value);               // Initialize by moving from an existing value

    // Copy/Move semantics for ShadowCopy itself
    ShadowCopy(const ShadowCopy&) = default;
    ShadowCopy(ShadowCopy&&) = default;
    ShadowCopy& operator=(const ShadowCopy&) = default;
    ShadowCopy& operator=(ShadowCopy&&) = default;

    // Accessing Data
    T& get();                            // Get mutable reference. Creates shadow if none exists. Marks as modified.
    const T& original() const;           // Get const reference to the original, immutable value.
    const T& current() const;            // Get const reference to the current value (shadow if exists, else original).

    // State Information
    bool modified() const;               // True if get() was called or if shadow exists and differs from original.
    bool has_shadow() const;             // True if a separate shadow copy currently exists.

    // State Management
    void commit();                       // Apply shadow changes to original. Clears shadow. Resets modified flag.
    void reset();                        // Discard shadow. Reverts to original. Resets modified flag.
    T take();                            // Move shadow value out. Clears shadow. Resets modified flag. (Returns T by value)
                                         // Throws std::logic_error if no shadow exists.
private:
    T original_;
    std::optional<T> shadow_;
    bool get_called_; // Internal flag to track if get() has been called
};
```
*(Note: `take()` is implemented to return `T` by value for safety, rather than `T&&` as originally sketched, to avoid dangling references when called on lvalue `ShadowCopy` objects.)*

## Lifecycle / State Transitions Overview

A `ShadowCopy<T>` object can be thought of in a few states:

1.  **Pristine State (Initial / After Commit / After Reset):**
    *   `has_shadow()` is `false`.
    *   `modified()` is `false`. (Unless `get()` is called immediately after construction without value change, then `modified()` is true but `has_shadow()` might still effectively be reflecting original content until actual different mutation)
    *   `current()` returns a reference to `original_`.

2.  **Shadowing State (After `get()` is called):**
    *   `get_called_` flag is `true`.
    *   `has_shadow()` is `true`. A copy of `original_` is now in `shadow_`.
    *   `modified()` is `true` (because `get()` was called). It remains `true` if the value in `shadow_` is changed to be different from `original_`.
    *   `current()` returns a reference to the value in `shadow_`.
    *   `get()` returns a mutable reference to the value in `shadow_`.

**Transitions:**

*   **`Pristine` -> `Shadowing`**:
    *   On first call to `get()`.
*   **`Shadowing` -> `Pristine` (Changes Applied)**:
    *   Call `commit()`: `original_` is updated from `shadow_`, `shadow_` is cleared. `get_called_` is reset.
*   **`Shadowing` -> `Pristine` (Changes Discarded)**:
    *   Call `reset()`: `shadow_` is cleared. `get_called_` is reset.
    *   Call `take()`: Value from `shadow_` is moved out, `shadow_` is cleared. `get_called_` is reset.

This provides a flexible way to manage temporary changes to objects.
The `modified()` flag is key: it becomes true if `get()` is called (signaling intent to modify or access mutably) OR if the shadow, once created, actually differs in value from the original.
This ensures that even if `get()` is called but no actual different data is written to the shadow, the object is still considered "touched" or "potentially modified". If the shadow's value is then changed back to be identical to the original, `modified()` would still be true if `get_called_` is true, but if `get_called_` was somehow false (e.g. after a commit/reset and then shadow recreated by other means not involving `get`), then `modified` would be false if values are same. The current logic `get_called_ || (has_shadow() && original_ != *shadow_)` covers this.

For the `modified()` logic:
- If `get()` has been called (`get_called_ == true`), then `modified()` is true.
- If `get()` has not been called (`get_called_ == false`), then `modified()` is true only if `has_shadow()` is true AND `*shadow_ != original_`. (This state is less common as `get()` is the primary way to create a shadow).

The most typical flow for `modified()`:
- Initially: `false`.
- After `get()`: `true`. (shadow created, `get_called_` is true)
- After `shadow.get().value = xyz;` (actual change): `true`.
- After `commit()` or `reset()`: `false`.
