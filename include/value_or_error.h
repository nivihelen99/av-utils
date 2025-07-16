#pragma once

#include <stdexcept>
#include <variant>

namespace cxxds {

template <typename T, typename E>
class value_or_error {
public:
    value_or_error(T value) : data_{std::move(value)} {}
    value_or_error(E error) : data_{std::move(error)} {}

    bool has_value() const {
        return std::holds_alternative<T>(data_);
    }

    bool has_error() const {
        return std::holds_alternative<E>(data_);
    }

    const T& value() const& {
        if (!has_value()) {
            throw std::logic_error("value_or_error does not contain a value");
        }
        return std::get<T>(data_);
    }

    T& value() & {
        if (!has_value()) {
            throw std::logic_error("value_or_error does not contain a value");
        }
        return std::get<T>(data_);
    }

    T&& value() && {
        if (!has_value()) {
            throw std::logic_error("value_or_error does not contain a value");
        }
        return std::move(std::get<T>(data_));
    }

    const E& error() const& {
        if (!has_error()) {
            throw std::logic_error("value_or_error does not contain an error");
        }
        return std::get<E>(data_);
    }

    E& error() & {
        if (!has_error()) {
            throw std::logic_error("value_or_error does not contain an error");
        }
        return std::get<E>(data_);
    }

    E&& error() && {
        if (!has_error()) {
            throw std::logic_error("value_or_error does not contain an error");
        }
        return std::move(std::get<E>(data_));
    }

private:
    std::variant<T, E> data_;
};

}  // namespace cxxds
