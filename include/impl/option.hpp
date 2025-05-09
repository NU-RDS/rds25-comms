#ifndef __OPTION_H__
#define __OPTION_H__

namespace comms {

template <typename T>
class Option {
   public:
    static Option<T> some(T value) { return Option<T>(value); }
    static Option<T> none() { return Option<T>(); }

    Option() : _hasValue(false) {}

    Option<T> operator=(const Option<T> &other) {
        _value = other._value;
        _hasValue = other._hasValue;
        return *this;
    }

    operator bool() const { return _hasValue; }

    bool isSome() const { return _hasValue; }
    bool isNone() const { return !_hasValue; }

    T value() const { return _value; }

   private:
    T _value;
    bool _hasValue;

    Option(T value) : _value(value), _hasValue(true) {}
};

}  // namespace comms

#endif  // __OPTION_H__
