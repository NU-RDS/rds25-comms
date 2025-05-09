#ifndef __RESULT_H__
#define __RESULT_H__

#include <sstream>
#include <functional>

namespace comms {

template <typename T>
class Result {
   public:
    static Result<T> ok(T value) { return Result<T>(value); }
    static Result<T> errorResult() { return Result<T>(true); }
    static Result<T> errorResult(std::string errorMessage) {
        return Result<T>(true, errorMessage);
    }

    operator bool() const { return !_error; }

    Result() : _error(true) {}

    Result<T> operator=(const Result<T> &other) {
        _value = other._value;
        _error = other._error;
        _errorMessage = other._errorMessage;
        return *this;
    }

    T value() const { return _value; }
    bool isError() const { return _error; }
    std::string error() const { return _errorMessage; }

   private:
    T _value;
    bool _error;
    std::string _errorMessage;

    Result(T value) : _value(value), _error(false), _errorMessage("") {}
    Result(bool error) : _error(error), _errorMessage(""), _value(T()) {}
    Result(bool error, std::string errorMessage)
        : _error(error), _errorMessage(errorMessage) {}
};

template <typename OnError, typename... Fs>
bool check(OnError onError, Fs &&...functions) {
    // check if any of the functions return an error
    bool error = false;
    std::vector<std::string> errors;
    std::initializer_list<int>{(error |= functions.isError(), 0)...};
    std::initializer_list<int>{(errors.push_back(functions.error()), 0)...};

    if (error) {
        // concatenate the error messages into one string
        std::string errorMessage;
        for (const std::string &error : errors) {
            if (error.empty()) {
                continue;
            }
            errorMessage += error;
            errorMessage += "\n";
        }

        onError(errorMessage.c_str());

        return true;
    }

    return false;
}

std::function<void(const char *)> defaultErrorCallback(std::ostream &stream) {
    return [&stream](const char *error) {
        stream << "Error: " << error << std::endl;
    };
}

}  // namespace comms

#endif  // __RESULT_H__