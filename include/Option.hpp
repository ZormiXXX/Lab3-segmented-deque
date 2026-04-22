#pragma once
#include <optional>
#include <stdexcept>

template<class T>
class Option {
private:
    std::optional<T> value;
    
    Option() : value(std::nullopt) {}
    
public:
    static Option<T> None() {
        return Option<T>();
    }
    
    static Option<T> Some(const T& val) {
        Option<T> opt;
        opt.value = val;
        return opt;
    }
    
    bool IsSome() const {
        return value.has_value();
    }
    
    bool IsNone() const {
        return !value.has_value();
    }
    
    T GetValue() const {
        if (!value.has_value()) {
            throw std::runtime_error("Trying to get value from None");
        }
        return value.value();
    }
    
    T GetValueOrDefault(const T& defaultVal) const {
        return value.value_or(defaultVal);
    }
};
