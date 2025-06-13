#ifndef __RESULT_H__
#define __RESULT_H__

#include <sstream>
#include <functional>

namespace comms {

/// @brief A class representing the result of an operation
/// @tparam T The type of the value returned on success
template <typename T>
class Result {
   public:

    /// @brief Creates a Result representing a successful operation with the given value
    /// @param value The value to return on success
    static Result<T> ok(T value) { return Result<T>(value); }

    /// @brief Creates a Result representing an error with no value
    /// @return A Result indicating an error
    static Result<T> errorResult() { return Result<T>(true); }

    /// @brief Creates a Result representing an error with a specific error message
    /// @param errorMessage The error message to return
    static Result<T> errorResult(std::string errorMessage) { return Result<T>(true, errorMessage); }

    /// @brief Converts the Result to a boolean indicating success or failure
    /// @return True if the operation was successful, false if it was an error
    operator bool() const { return !_error; }

    /// @brief Default constructor for Result, initializes as an error
    Result() : _error(true) {}

    /// @brief Copy constructor for Result
    Result<T> operator=(const Result<T>& other) {
        _value = other._value;
        _error = other._error;
        _errorMessage = other._errorMessage;
        return *this;
    }

    /// @brief Gets the value of the Result
    /// @note This should only be called if isError() returns false
    T value() const { return _value; }

    /// @brief Checks if the Result represents an error
    /// @return True if the Result is an error, false if it is successful
    bool isError() const { return _error; }

    /// @brief Gets the error message if the Result is an error
    /// @return The error message, or an empty string if there is no error
    std::string error() const { return _errorMessage; }

   private:
    T _value;
    bool _error;
    std::string _errorMessage;

    Result(T value) : _value(value), _error(false), _errorMessage("") {}
    Result(bool error) : _error(error), _errorMessage(""), _value(T()) {}
    Result(bool error, std::string errorMessage) : _error(error), _errorMessage(errorMessage) {}
};

/// @brief Checks if any of the provided functions return an error
/// @tparam OnError The type of the error handling function
/// @tparam Fs The types of the functions to check
template <typename OnError, typename... Fs>
bool check(OnError onError, Fs&&... functions) {
    // check if any of the functions return an error
    bool error = false;
    std::vector<std::string> errors;
    std::initializer_list<int>{(error |= functions.isError(), 0)...};
    std::initializer_list<int>{(errors.push_back(functions.error()), 0)...};

    if (error) {
        // concatenate the error messages into one string
        std::string errorMessage;
        for (const std::string& error : errors) {
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

// std::function<void(const char *)> defaultErrorCallback(std::ostream &stream) {
//     return [&stream](const char *error) {
//         stream << "Error: " << error << std::endl;
//     };
// }

}  // namespace comms

#endif  // __RESULT_H__