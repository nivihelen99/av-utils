#ifndef AOSX_L2_RETURN_H
#define AOSX_L2_RETURN_H

#include <iostream>
#include <map>
#include <ostream>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <variant>
#include <functional>

namespace aos_utils
{

// Forward declarations
template <typename E_tpl> class Unexpected; // Renamed to avoid conflict with E in Expected
template <typename T, typename E_cls> class Expected; // Primary template, E_cls to avoid conflict
template <typename E_cls_void> class Expected<void, E_cls_void>;   // Specialization for void

// Trait to check if a type is an Unexpected
template<typename T_unex> struct is_unexpected_trait : std::false_type {};
template<typename E_tpl> struct is_unexpected_trait<Unexpected<E_tpl>> : std::true_type {};
template<typename T_unex> constexpr bool is_unexpected_trait_v = is_unexpected_trait<T_unex>::value;

// Trait to check if a type is an Expected
template<typename T_ex> struct is_expected_trait : std::false_type {};
template<typename T_val, typename E_err> struct is_expected_trait<Expected<T_val,E_err>> : std::true_type {};
template<typename T_ex> constexpr bool is_expected_trait_v = is_expected_trait<T_ex>::value;


template <typename E_tpl>
class Unexpected
{
public:
    using error_type = E_tpl;

    template <typename U = E_tpl,
              typename = std::enable_if_t<std::is_constructible_v<E_tpl, U> &&
                                        !is_unexpected_trait_v<std::decay_t<U>>>>
    explicit constexpr Unexpected(U&& error) noexcept(std::is_nothrow_constructible_v<E_tpl, U>)
        : m_error(std::forward<U>(error))
    {
    }

    constexpr const E_tpl& error() const& noexcept { return m_error; }
    constexpr E_tpl& error() & noexcept { return m_error; }
    constexpr E_tpl&& error() && noexcept { return std::move(m_error); }
    constexpr const E_tpl&& error() const&& noexcept { return std::move(m_error); }

    template <typename E2>
    constexpr bool operator==(const Unexpected<E2>& other) const noexcept(noexcept(m_error == other.error()))
    {
        return m_error == other.error();
    }
    template <typename E2>
    constexpr bool operator!=(const Unexpected<E2>& other) const noexcept(noexcept(!(*this == other)))
    {
        return !(*this == other);
    }
private:
    E_tpl m_error;
};

template <typename E_tpl> Unexpected(E_tpl) -> Unexpected<E_tpl>;


template <typename T, typename E_cls = std::string>
class Expected // Primary Template (T is not void)
{
public:
    using value_type = T;
    using error_type = E_cls;
    using unexpected_type = Unexpected<E_cls>;

    static_assert(!std::is_void_v<T>, "Primary Expected template is for non-void T.");
    static_assert(!std::is_reference_v<T>, "T must not be a reference type.");
    static_assert(!std::is_same_v<T, std::in_place_t>, "T must not be std::in_place_t.");

    template <typename U = T, typename = std::enable_if_t<std::is_default_constructible_v<U>>>
    constexpr Expected() noexcept(std::is_nothrow_default_constructible_v<T>)
        : m_value_or_error(std::in_place_index<0>, T{}) {}

    template <typename U = T,
              typename = std::enable_if_t<
                  std::is_constructible_v<T, U> &&
                  !is_expected_trait_v<std::decay_t<U>> &&
                  !is_unexpected_trait_v<std::decay_t<U>> &&
                  !std::is_same_v<std::decay_t<U>, std::in_place_t>>>
    constexpr Expected(U&& value) noexcept(std::is_nothrow_constructible_v<T, U>)
        : m_value_or_error(std::in_place_index<0>, std::forward<U>(value)) {}

    template <typename G>
    constexpr Expected(const Unexpected<G>& unexpected) noexcept(std::is_nothrow_constructible_v<E_cls, const G&>)
        : m_value_or_error(std::in_place_index<1>, unexpected.error()) {}

    template <typename G>
    constexpr Expected(Unexpected<G>&& unexpected) noexcept(std::is_nothrow_constructible_v<E_cls, G&&>)
        : m_value_or_error(std::in_place_index<1>, std::move(unexpected.error())) {}

    template <typename... Args>
    constexpr explicit Expected(std::in_place_t, Args&&... args) noexcept(std::is_nothrow_constructible_v<T, Args...>)
        : m_value_or_error(std::in_place_index<0>, std::forward<Args>(args)...) {}

    template <typename U_E, typename... Args>
    constexpr explicit Expected(std::in_place_type_t<Unexpected<U_E>>, Args&&... args) noexcept(std::is_nothrow_constructible_v<E_cls, Args...>)
        : m_value_or_error(std::in_place_index<1>, std::forward<Args>(args)...) {
         static_assert(std::is_same_v<U_E, E_cls>, "Unexpected type in in_place_type_t must match Expected's error_type");
    }

    Expected(const Expected&) = default;
    Expected(Expected&&) = default;
    Expected& operator=(const Expected&) = default;
    Expected& operator=(Expected&&) = default;

    template <typename U = T,
              typename = std::enable_if_t<
                  std::is_constructible_v<T, U> && std::is_assignable_v<T&, U> &&
                  !is_expected_trait_v<std::decay_t<U>> && !is_unexpected_trait_v<std::decay_t<U>>>>
    Expected& operator=(U&& value) noexcept(std::is_nothrow_constructible_v<T, U> && std::is_nothrow_assignable_v<T&, U>) {
        if (has_value()) std::get<0>(m_value_or_error) = std::forward<U>(value);
        else m_value_or_error.template emplace<0>(std::forward<U>(value));
        return *this;
    }
    template <typename G> Expected& operator=(const Unexpected<G>& unex) { m_value_or_error.template emplace<1>(unex.error()); return *this; }
    template <typename G> Expected& operator=(Unexpected<G>&& unex) { m_value_or_error.template emplace<1>(std::move(unex.error())); return *this; }

    ~Expected() = default;

    constexpr bool has_value() const noexcept { return m_value_or_error.index() == 0; }
    constexpr explicit operator bool() const noexcept { return has_value(); }

    constexpr const T& value() const& { if (!has_value()) throw std::bad_variant_access{}; return std::get<0>(m_value_or_error); }
    constexpr T& value() & { if (!has_value()) throw std::bad_variant_access{}; return std::get<0>(m_value_or_error); }
    constexpr T&& value() && { if (!has_value()) throw std::bad_variant_access{}; return std::move(std::get<0>(m_value_or_error)); }
    constexpr const T&& value() const&& { if (!has_value()) throw std::bad_variant_access{}; return std::move(std::get<0>(m_value_or_error)); }

    constexpr const E_cls& error() const& { if (has_value()) throw std::bad_variant_access{}; return std::get<1>(m_value_or_error); }
    constexpr E_cls& error() & { if (has_value()) throw std::bad_variant_access{}; return std::get<1>(m_value_or_error); }
    constexpr E_cls&& error() && { if (has_value()) throw std::bad_variant_access{}; return std::move(std::get<1>(m_value_or_error)); }
    constexpr const E_cls&& error() const&& { if (has_value()) throw std::bad_variant_access{}; return std::move(std::get<1>(m_value_or_error)); }

    constexpr const T& operator*() const& noexcept { return std::get<0>(m_value_or_error); }
    constexpr T& operator*() & noexcept { return std::get<0>(m_value_or_error); }
    constexpr T&& operator*() && noexcept { return std::move(std::get<0>(m_value_or_error)); }
    constexpr const T&& operator*() const&& noexcept { return std::move(std::get<0>(m_value_or_error)); }

    constexpr const T* operator->() const noexcept { return &std::get<0>(m_value_or_error); }
    constexpr T* operator->() noexcept { return &std::get<0>(m_value_or_error); }

    template <typename UVal> constexpr T value_or(UVal&& dv) const& { return has_value() ? **this : static_cast<T>(std::forward<UVal>(dv)); }
    template <typename UVal> constexpr T value_or(UVal&& dv) && { return has_value() ? std::move(**this) : static_cast<T>(std::forward<UVal>(dv)); }

    template <typename F> constexpr auto map(F&& f) const& {
        using U = std::invoke_result_t<F, const T&>;
        if (has_value()) {
            if constexpr (std::is_void_v<U>) { std::invoke(std::forward<F>(f), **this); return Expected<void, E_cls>(std::in_place); }
            else { return Expected<U, E_cls>(std::invoke(std::forward<F>(f), **this)); }
        } return Expected<U, E_cls>(Unexpected(error()));
    }
    template <typename F> constexpr auto map(F&& f) && {
        using U = std::invoke_result_t<F, T&&>;
        if (has_value()) {
            if constexpr (std::is_void_v<U>) { std::invoke(std::forward<F>(f), std::move(**this)); return Expected<void, E_cls>(std::in_place); }
            else { return Expected<U, E_cls>(std::invoke(std::forward<F>(f), std::move(**this))); }
        } return Expected<U, E_cls>(Unexpected(std::move(error())));
    }
    template <typename F> constexpr auto and_then(F&& f) const& {
        using U = std::invoke_result_t<F, const T&>;
        static_assert(is_expected_trait_v<U>, "F must return Expected"); static_assert(std::is_same_v<typename U::error_type, E_cls>, "Error types must match");
        if (has_value()) return std::invoke(std::forward<F>(f), **this); return U(Unexpected(error()));
    }
    template <typename F> constexpr auto and_then(F&& f) && {
        using U = std::invoke_result_t<F, T&&>;
        static_assert(is_expected_trait_v<U>, "F must return Expected"); static_assert(std::is_same_v<typename U::error_type, E_cls>, "Error types must match");
        if (has_value()) return std::invoke(std::forward<F>(f), std::move(**this)); return U(Unexpected(std::move(error())));
    }
    template <typename F> constexpr auto or_else(F&& f) const& -> std::invoke_result_t<F, const E_cls&> {
        using NextExpected = std::invoke_result_t<F, const E_cls&>;
        static_assert(is_expected_trait_v<NextExpected>, "F must return an Expected type for or_else");
        static_assert(std::is_constructible_v<NextExpected, const T&>, "Result type must be constructible from T");
        if (has_value()) return NextExpected(**this); return std::invoke(std::forward<F>(f), error());
    }
    template <typename F> constexpr auto or_else(F&& f) && -> std::invoke_result_t<F, E_cls&&> {
        using NextExpected = std::invoke_result_t<F, E_cls&&>;
        static_assert(is_expected_trait_v<NextExpected>, "F must return an Expected type for or_else");
        static_assert(std::is_constructible_v<NextExpected, T&&>, "Result type must be constructible from T&&");
        if (has_value()) return NextExpected(std::move(**this)); return std::invoke(std::forward<F>(f), std::move(error()));
    }
    template <typename F> constexpr auto map_error(F&& f) const& -> Expected<T, std::invoke_result_t<F, const E_cls&>> {
        using G = std::invoke_result_t<F, const E_cls&>;
        if (has_value()) return Expected<T, G>(std::in_place, **this);
        return Expected<T, G>(Unexpected(std::invoke(std::forward<F>(f), error())));
    }
    template <typename F> constexpr auto map_error(F&& f) && -> Expected<T, std::invoke_result_t<F, E_cls&&>> {
        using G = std::invoke_result_t<F, E_cls&&>;
        if (has_value()) return Expected<T, G>(std::in_place, std::move(**this));
        return Expected<T, G>(Unexpected(std::invoke(std::forward<F>(f), std::move(error()))));
    }

    void swap(Expected& other) noexcept(std::is_nothrow_move_constructible_v<T> && std::is_nothrow_swappable_v<T> && std::is_nothrow_move_constructible_v<E_cls> && std::is_nothrow_swappable_v<E_cls>) {
        using std::swap; swap(m_value_or_error, other.m_value_or_error);
    }
    template <typename T2, typename E2> constexpr bool operator==(const Expected<T2, E2>& other) const {
        if (has_value() != other.has_value()) return false;
        return has_value() ? (**this == *other) : (error() == other.error());
    }
    template <typename T2, typename E2> constexpr bool operator!=(const Expected<T2, E2>& other) const { return !(*this == other); }
    template <typename T2> constexpr bool operator==(const T2& v) const { return has_value() && (**this == v); }
    template <typename T2> constexpr bool operator!=(const T2& v) const { return !(*this == v); }
    template <typename E2> constexpr bool operator==(const Unexpected<E2>& u) const { return !has_value() && (error() == u.error()); }
    template <typename E2> constexpr bool operator!=(const Unexpected<E2>& u) const { return !(*this == u); }
private:
    std::variant<T, E_cls> m_value_or_error;
};

template <typename E_cls>
class Expected<void, E_cls> {
public:
    using value_type = void;
    using error_type = E_cls;
    using unexpected_type = Unexpected<E_cls>;

    constexpr Expected() noexcept : m_has_error(false) {}
    constexpr Expected(std::in_place_t) noexcept : m_has_error(false) {}

    template <typename G>
    constexpr Expected(const Unexpected<G>& unexpected) noexcept(std::is_nothrow_constructible_v<E_cls, const G&>)
        : m_has_error(true) {
        new (&m_error_storage) E_cls(unexpected.error());
    }
    template <typename G>
    constexpr Expected(Unexpected<G>&& unexpected) noexcept(std::is_nothrow_constructible_v<E_cls, G&&>)
        : m_has_error(true) {
        new (&m_error_storage) E_cls(std::move(unexpected.error()));
    }
    template <typename U_E, typename... Args>
    constexpr explicit Expected(std::in_place_type_t<Unexpected<U_E>>, Args&&... args) noexcept(std::is_nothrow_constructible_v<E_cls, Args...>)
        : m_has_error(true) {
        static_assert(std::is_same_v<U_E, E_cls>, "Unexpected type mismatch");
        new (&m_error_storage) E_cls(std::forward<Args>(args)...);
    }

    Expected(const Expected& other) noexcept(std::is_nothrow_copy_constructible_v<E_cls>)
        : m_has_error(other.m_has_error) {
        if (m_has_error) new (&m_error_storage) E_cls(other.error());
    }
    Expected(Expected&& other) noexcept(std::is_nothrow_move_constructible_v<E_cls>)
        : m_has_error(other.m_has_error) {
        if (m_has_error) {
            new (&m_error_storage) E_cls(std::move(other.error()));
            // other.m_has_error = false; // Or other becomes valueless if error moved from
        }
    }
    Expected& operator=(const Expected& other) noexcept(std::is_nothrow_copy_constructible_v<E_cls> && std::is_nothrow_copy_assignable_v<E_cls>) {
        if (this == &other) return *this;
        destroy_error_if_present();
        m_has_error = other.m_has_error;
        if (m_has_error) new (&m_error_storage) E_cls(other.error());
        return *this;
    }
    Expected& operator=(Expected&& other) noexcept(std::is_nothrow_move_constructible_v<E_cls> && std::is_nothrow_move_assignable_v<E_cls>) {
        if (this == &other) return *this;
        destroy_error_if_present();
        m_has_error = other.m_has_error;
        if (m_has_error) {
            new (&m_error_storage) E_cls(std::move(other.error()));
            // other.m_has_error = false; // Or other becomes valueless
        }
        return *this;
    }
    Expected& operator=(std::in_place_t) noexcept { destroy_error_if_present(); m_has_error = false; return *this; }
    template <typename G> Expected& operator=(const Unexpected<G>& unex) {
        destroy_error_if_present(); new (&m_error_storage) E_cls(unex.error()); m_has_error = true; return *this;
    }
    template <typename G> Expected& operator=(Unexpected<G>&& unex) {
        destroy_error_if_present(); new (&m_error_storage) E_cls(std::move(unex.error())); m_has_error = true; return *this;
    }

    ~Expected() { destroy_error_if_present(); }

    constexpr bool has_value() const noexcept { return !m_has_error; }
    constexpr explicit operator bool() const noexcept { return has_value(); }

    constexpr void value() const& { if (!has_value()) throw std::bad_variant_access{}; }
    constexpr void value() & { if (!has_value()) throw std::bad_variant_access{}; }
    constexpr void value() && { if (!has_value()) throw std::bad_variant_access{}; }
    constexpr void value() const&& { if (!has_value()) throw std::bad_variant_access{}; }

    constexpr const E_cls& error() const& { if (has_value()) throw std::bad_variant_access{}; return *reinterpret_cast<const E_cls*>(&m_error_storage); }
    constexpr E_cls& error() & { if (has_value()) throw std::bad_variant_access{}; return *reinterpret_cast<E_cls*>(&m_error_storage); }
    constexpr E_cls&& error() && { if (has_value()) throw std::bad_variant_access{}; return std::move(*reinterpret_cast<E_cls*>(&m_error_storage)); }
    constexpr const E_cls&& error() const&& { if (has_value()) throw std::bad_variant_access{}; return std::move(*reinterpret_cast<const E_cls*>(&m_error_storage)); }

    template <typename F> constexpr auto map(F&& f) const& {
        using U = std::invoke_result_t<F>;
        if (has_value()) {
            if constexpr (std::is_void_v<U>) { std::invoke(std::forward<F>(f)); return Expected<void, E_cls>(std::in_place); }
            else { return Expected<U, E_cls>(std::invoke(std::forward<F>(f))); }
        } return Expected<U, E_cls>(Unexpected(error()));
    }
    template <typename F> constexpr auto map(F&& f) && {
        using U = std::invoke_result_t<F>;
        if (has_value()) {
            if constexpr (std::is_void_v<U>) { std::invoke(std::forward<F>(f)); return Expected<void, E_cls>(std::in_place); }
            else { return Expected<U, E_cls>(std::invoke(std::forward<F>(f))); }
        } return Expected<U, E_cls>(Unexpected(std::move(error())));
    }
    template <typename F> constexpr auto and_then(F&& f) const& {
        using NextExpected = std::invoke_result_t<F>;
        static_assert(is_expected_trait_v<NextExpected>, "F must return an Expected type");
        static_assert(std::is_same_v<typename NextExpected::error_type, E_cls>, "Error types must match");
        if (has_value()) return std::invoke(std::forward<F>(f));
        return NextExpected(Unexpected(error()));
    }
    template <typename F> constexpr auto and_then(F&& f) && {
        using NextExpected = std::invoke_result_t<F>;
        static_assert(is_expected_trait_v<NextExpected>, "F must return an Expected type");
        static_assert(std::is_same_v<typename NextExpected::error_type, E_cls>, "Error types must match");
        if (has_value()) return std::invoke(std::forward<F>(f));
        return NextExpected(Unexpected(std::move(error())));
    }
    template <typename F> constexpr auto or_else(F&& f) const& -> std::invoke_result_t<F, const E_cls&> {
        using NextExpected = std::invoke_result_t<F, const E_cls&>;
        static_assert(is_expected_trait_v<NextExpected>, "F must return an Expected type for or_else");
        static_assert(std::is_void_v<typename NextExpected::value_type>, "Value type of NextExpected must be void");
        if (has_value()) return NextExpected(std::in_place);
        return std::invoke(std::forward<F>(f), error());
    }
    template <typename F> constexpr auto or_else(F&& f) && -> std::invoke_result_t<F, E_cls&&> {
        using NextExpected = std::invoke_result_t<F, E_cls&&>;
        static_assert(is_expected_trait_v<NextExpected>, "F must return an Expected type for or_else");
        static_assert(std::is_void_v<typename NextExpected::value_type>, "Value type of NextExpected must be void");
        if (has_value()) return NextExpected(std::in_place);
        return std::invoke(std::forward<F>(f), std::move(error()));
    }
    template <typename F> constexpr auto map_error(F&& f) const& -> Expected<void, std::invoke_result_t<F, const E_cls&>> {
        using G = std::invoke_result_t<F, const E_cls&>;
        if (has_value()) return Expected<void, G>(std::in_place);
        return Expected<void, G>(Unexpected(std::invoke(std::forward<F>(f), error())));
    }
    template <typename F> constexpr auto map_error(F&& f) && -> Expected<void, std::invoke_result_t<F, E_cls&&>> {
        using G = std::invoke_result_t<F, E_cls&&>;
        if (has_value()) return Expected<void, G>(std::in_place);
        return Expected<void, G>(Unexpected(std::invoke(std::forward<F>(f), std::move(error()))));
    }

    void swap(Expected<void, E_cls>& other) noexcept(std::is_nothrow_move_constructible_v<E_cls> && std::is_nothrow_swappable_v<E_cls>) {
        if (has_value() && other.has_value()) { /* Both void, no state change */ }
        else if (has_value() && !other.has_value()) { // this has value, other has error
            new (&m_error_storage) E_cls(std::move(other.error()));
            other.destroy_error_if_present();
            m_has_error = true; other.m_has_error = false;
        } else if (!has_value() && other.has_value()) { // this has error, other has value
            new (&other.m_error_storage) E_cls(std::move(error()));
            destroy_error_if_present();
            m_has_error = false; other.m_has_error = true;
        } else { // Both have errors
            using std::swap; swap(error(), other.error());
        }
    }

    template <typename E2> constexpr bool operator==(const Expected<void, E2>& other) const {
        return has_value() == other.has_value() && (has_value() || error() == other.error());
    }
    template <typename E2> constexpr bool operator!=(const Expected<void, E2>& other) const { return !(*this == other); }
    template <typename E2> constexpr bool operator==(const Unexpected<E2>& u) const { return !has_value() && (error() == u.error()); }
    template <typename E2> constexpr bool operator!=(const Unexpected<E2>& u) const { return !(*this == u); }
private:
    union ErrorStorage {
        constexpr ErrorStorage() : dummy_{} {} // Initialize dummy for safety if E is not trivial
        ~ErrorStorage() {}
        char dummy_; // Ensure union is not empty
        E_cls error_val;
    } m_error_storage;
    bool m_has_error;

    void destroy_error_if_present() noexcept {
        if (m_has_error) {
            if constexpr (!std::is_trivially_destructible_v<E_cls>) {
                 reinterpret_cast<E_cls*>(&m_error_storage.error_val)->~E_cls();
            }
        }
    }
};

// Deduction guides
template<class TVal,
         class = std::enable_if_t<
             !std::is_same_v<std::decay_t<TVal>, std::in_place_t> &&
             !is_unexpected_trait_v<std::decay_t<TVal>> && // Use trait
             !is_expected_trait_v<std::decay_t<TVal>>    // Use trait
         >>
Expected(TVal&&) -> Expected<TVal, std::string>; // Default E

template<class E_tpl> Expected(const Unexpected<E_tpl>&) -> Expected<void, E_tpl>;
template<class E_tpl> Expected(Unexpected<E_tpl>&&) -> Expected<void, E_tpl>;

// Non-member functions
template <typename E_Type>
constexpr Unexpected<std::decay_t<E_Type>> make_unexpected(E_Type&& error)
    noexcept(std::is_nothrow_constructible_v<std::decay_t<E_Type>, E_Type>) {
    return Unexpected<std::decay_t<E_Type>>(std::forward<E_Type>(error));
}

template <typename Func, typename DefErr = std::string>
auto make_expected(Func&& func) noexcept -> Expected<std::invoke_result_t<Func>, DefErr> {
    try {
        if constexpr (std::is_void_v<std::invoke_result_t<Func>>) {
            std::invoke(std::forward<Func>(func));
            return Expected<void, DefErr>{std::in_place};
        } else {
            return Expected<std::invoke_result_t<Func>, DefErr>{std::invoke(std::forward<Func>(func))};
        }
    }
    catch (const std::exception& e) {
        return Expected<std::invoke_result_t<Func>, DefErr>{make_unexpected(DefErr{e.what()})};
    }
    catch (...) {
        return Expected<std::invoke_result_t<Func>, DefErr>{make_unexpected(DefErr{"Unknown exception"})};
    }
}

template <typename T, typename E_cls>
void swap(Expected<T, E_cls>& lhs, Expected<T, E_cls>& rhs)
    noexcept(noexcept(lhs.swap(rhs))) {
    lhs.swap(rhs);
}

} // namespace aos_utils
#endif
