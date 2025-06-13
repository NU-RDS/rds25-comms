#ifndef __OPTION_H__
#define __OPTION_H__

namespace comms {

/// @brief A class representing an optional value
/// @tparam T The type of the value
template <typename T>
class Option {
   public:
    /// @brief Creates an Option containing a value
    /// @param value The value to store in the Option
    /// @return An Option containing the value
    static Option<T> some(T value) { return Option<T>(value); }

    /// @brief Creates an Option representing no value
    /// @return An Option representing no value
    static Option<T> none() { return Option<T>(); }

    /// @brief Default constructor for Option, initializes as none
    /// @note This means the Option does not contain a value
    Option() : _hasValue(false) {}

    Option<T> operator=(const Option<T>& other) {
        _value = other._value;
        _hasValue = other._hasValue;
        return *this;
    }

    /// @brief Converts the Option to a boolean indicating if it contains a value
    /// @return True if the Option contains a value, false if it is none
    operator bool() const { return _hasValue; }

    /// @brief Checks if the Option contains a value
    /// @return True if the Option contains a value, false if it is none
    bool isSome() const { return _hasValue; }

    /// @brief Checks if the Option does not contain a value
    /// @return True if the Option does not contain a value, false if it is some
    bool isNone() const { return !_hasValue; }

    /// @brief Gets the value contained in the Option
    /// @note This should only be called if isSome() returns true
    T value() const { return _value; }

   private:
    T _value;
    bool _hasValue;

    Option(T value) : _value(value), _hasValue(true) {}
};

}  // namespace comms

#endif  // __OPTION_H__
