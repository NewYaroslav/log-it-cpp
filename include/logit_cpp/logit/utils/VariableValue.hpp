#pragma once
#ifndef _LOGIT_VARIABLE_VALUE_HPP_INCLUDED
#define _LOGIT_VARIABLE_VALUE_HPP_INCLUDED

/// \file VariableValue.hpp
/// \brief Structure for storing variables of various types.

#include <time_shield/time_formatting.hpp>
#include <string>
#include <iostream>
#include <cstdint>
#include <type_traits>
#include <exception>
#include <iomanip> // std::put_time
#include <chrono>
#include <sstream>
#include <memory>
#if __cplusplus >= 201703L
#include <filesystem>
#include <optional>
#include <variant>
#endif

namespace logit {

    /// \brief Helper function to convert an enumeration to a string.
    /// \tparam EnumType The enumeration type.
    /// \param value The enumeration value.
    /// \return String representation of the enumeration.
    template <typename EnumType>
    std::string enum_to_string(EnumType value) {
        // Convert enum to underlying integral value and then to string.
        typedef typename std::underlying_type<EnumType>::type UnderlyingType;
        return std::to_string(static_cast<UnderlyingType>(value));
    }

    /// \struct VariableValue
    /// \brief Structure for storing values of various types, including enumerations.
    struct VariableValue {
        std::string name;       ///< Variable name.
        bool is_literal;        ///< Flag indicating if the variable is a literal.

        /// \enum ValueType
        /// \brief Enumeration of possible value types for VariableValue.
        enum class ValueType {
            INT8_VAL,           ///< Value of type `int8_t` (signed 8-bit integer).
            UINT8_VAL,          ///< Value of type `uint8_t` (unsigned 8-bit integer).
            INT16_VAL,          ///< Value of type `int16_t` (signed 16-bit integer).
            UINT16_VAL,         ///< Value of type `uint16_t` (unsigned 16-bit integer).
            INT32_VAL,          ///< Value of type `int32_t` (signed 32-bit integer).
            UINT32_VAL,         ///< Value of type `uint32_t` (unsigned 32-bit integer).
            INT64_VAL,          ///< Value of type `int64_t` (signed 64-bit integer).
            UINT64_VAL,         ///< Value of type `uint64_t` (unsigned 64-bit integer).
            BOOL_VAL,           ///< Value of type `bool`.
            CHAR_VAL,           ///< Value of type `char` (single character).
            FLOAT_VAL,          ///< Value of type `float` (single-precision floating point).
            DOUBLE_VAL,         ///< Value of type `double` (double-precision floating point).
            LONG_DOUBLE_VAL,    ///< Value of type `long double` (extended-precision floating point).
            DURATION_VAL,       ///< Value of type `std::chrono::duration` (time duration).
            TIME_POINT_VAL,     ///< Value of type `std::chrono::time_point` (specific point in time).
            STRING_VAL,         ///< Value of type `std::string` (dynamic-length string).
            EXCEPTION_VAL,      ///< Value representing an exception (derived from `std::exception`).
            ERROR_CODE_VAL,     ///< Value of type `std::error_code` (system error code).
            ENUM_VAL,           ///< Value of any enumeration type (converted to string or integral value).
            PATH_VAL,           ///< Value of type `std::filesystem::path` (filesystem path).
            POINTER_VAL,        ///< Value of type `void*` (raw pointer).
            SMART_POINTER_VAL,  ///< Value of type `std::shared_ptr` or `std::unique_ptr` (smart pointers).
            VARIANT_VAL,        ///< Value of type `std::variant` (type-safe union).
            OPTIONAL_VAL,       ///< Value of type `std::optional` (optional value holder).
            UNKNOWN_VAL         ///< Unknown or unsupported value type.
        } type;                 ///< Specifies the type of the stored value in the VariableValue structure.

        union {
            int8_t     int8_value;
            uint8_t    uint8_value;
            int16_t    int16_value;
            uint16_t   uint16_value;
            int32_t    int32_value;
            uint32_t   uint32_value;
            int64_t    int64_value;
            uint64_t   uint64_value;
            bool       bool_value;
            char       char_value;
            float      float_value;
            double     double_value;
            long double long_double_value;
        } pod_value; ///< Union to store POD types.

        std::string string_value;           ///< Variable to store string, exception messages, and enums.
        std::error_code error_code_value;   ///< Variable to store std::error_code.

        // Constructors for each type.
        template <typename T>
        VariableValue(const std::string& name, T value,
                      typename std::enable_if<std::is_same<T, bool>::value>::type* = nullptr)
            : name(name), is_literal(is_valid_literal_name(name)), type(ValueType::BOOL_VAL) {
            pod_value.bool_value = value;
        }

        template <typename T>
        VariableValue(const std::string& name, T value,
                      typename std::enable_if<std::is_same<T, char>::value>::type* = nullptr)
            : name(name), is_literal(is_valid_literal_name(name)), type(ValueType::CHAR_VAL) {
            pod_value.char_value = value;
        }

        explicit VariableValue(const std::string& name, const std::string& value)
            : name(name), is_literal(is_valid_literal_name(name)), type(ValueType::STRING_VAL), string_value(value) {
        }

        explicit VariableValue(const std::string& name, const char* value) :
            VariableValue(name, std::string(value)) {}

        template <typename T>
        VariableValue(const std::string& name, const T& value,
                      typename std::enable_if<std::is_base_of<std::exception, T>::value>::type* = nullptr)
            : name(name), is_literal(is_valid_literal_name(name)), type(ValueType::EXCEPTION_VAL), string_value(value.what()) {
        }

        explicit VariableValue(const std::string& name, const std::error_code& ec)
            : name(name), is_literal(is_valid_literal_name(name)), type(ValueType::ERROR_CODE_VAL), error_code_value(ec) {
        }

        template <typename T>
        VariableValue(const std::string& name, T value,
            typename std::enable_if<std::is_floating_point<T>::value>::type* = nullptr)
            : name(name), is_literal(is_valid_literal_name(name)), type(ValueType::UNKNOWN_VAL) {
            if (std::is_same<T, float>::value) {
                type = ValueType::FLOAT_VAL;
                pod_value.float_value = static_cast<float>(value);
            } else if (std::is_same<T, double>::value) {
                type = ValueType::DOUBLE_VAL;
                pod_value.double_value = static_cast<double>(value);
            } else if (std::is_same<T, long double>::value) {
                type = ValueType::LONG_DOUBLE_VAL;
                pod_value.long_double_value = static_cast<long double>(value);
            }
        }
        
#ifdef _MSC_VER
#   pragma warning(push)
#   pragma warning(disable : 4127)
#endif

        template <typename T>
        VariableValue(const std::string& name, T value,
            typename std::enable_if<
                std::is_integral<T>::value && !std::is_same<T, bool>::value
            >::type* = nullptr)
            : name(name), is_literal(is_valid_literal_name(name)), type(ValueType::UNKNOWN_VAL) {
            
            if (std::is_signed<T>::value) {
                if (sizeof(T) <= sizeof(int8_t)) {
                    type = ValueType::INT8_VAL;
                    pod_value.int8_value = static_cast<int8_t>(value);
                } else if (sizeof(T) <= sizeof(int16_t)) {
                    type = ValueType::INT16_VAL;
                    pod_value.int16_value = static_cast<int16_t>(value);
                } else if (sizeof(T) <= sizeof(int32_t)) {
                    type = ValueType::INT32_VAL;
                    pod_value.int32_value = static_cast<int32_t>(value);
                } else {
                    type = ValueType::INT64_VAL;
                    pod_value.int64_value = static_cast<int64_t>(value);
                }
            } else {
                if (sizeof(T) <= sizeof(uint8_t)) {
                    type = ValueType::UINT8_VAL;
                    pod_value.uint8_value = static_cast<uint8_t>(value);
                } else if (sizeof(T) <= sizeof(uint16_t)) {
                    type = ValueType::UINT16_VAL;
                    pod_value.uint16_value = static_cast<uint16_t>(value);
                } else if (sizeof(T) <= sizeof(uint32_t)) {
                    type = ValueType::UINT32_VAL;
                    pod_value.uint32_value = static_cast<uint32_t>(value);
                } else {
                    type = ValueType::UINT64_VAL;
                    pod_value.uint64_value = static_cast<uint64_t>(value);
                }
            }
        }
       
#ifdef _MSC_VER    
#   pragma warning(pop)
#endif

        /// \brief Constructor for enumerations.
        /// \tparam EnumType The enumeration type.
        /// \param name The variable name.
        /// \param value The enumeration value.
        template <typename EnumType>
        VariableValue(const std::string& name, EnumType value,
            typename std::enable_if<std::is_enum<EnumType>::value>::type* = 0)
            : name(name), is_literal(is_valid_literal_name(name)),
              type(ValueType::ENUM_VAL), string_value(enum_to_string(value)) {
        }

        template <typename Rep, typename Period>
        VariableValue(const std::string& name, const std::chrono::duration<Rep, Period>& duration)
            : name(name), is_literal(is_valid_literal_name(name)), type(ValueType::DURATION_VAL) {
            string_value = std::to_string(duration.count()) + " " + duration_units<Period>();
        }

        template <typename Clock, typename Duration>
        VariableValue(const std::string& name, const std::chrono::time_point<Clock, Duration>& time_point)
            : name(name), is_literal(is_valid_literal_name(name)), type(ValueType::TIME_POINT_VAL) {
            auto ts_ms = std::chrono::duration_cast<std::chrono::milliseconds>(time_point.time_since_epoch());
            //string_value = time_shield::to_human_readable_ms(ts_ms.count());
        }

#       if __cplusplus >= 201703L

        explicit VariableValue(const std::string& name, const std::filesystem::path& path)
            : name(name), is_literal(is_valid_literal_name(name)), type(ValueType::PATH_VAL), string_value(path.string()) {
        }

        template <typename... Ts>
        explicit VariableValue(const std::string& name, const std::variant<Ts...>& variant)
            : name(name), is_literal(is_valid_literal_name(name)), type(ValueType::VARIANT_VAL) {
            string_value = std::visit([](const auto& value) -> std::string {
                if constexpr (std::is_arithmetic_v<decltype(value)>) {
                    return std::to_string(value);
                } else if constexpr (std::is_same_v<decltype(value), std::string>) {
                    return value;
                } else {
                    std::ostringstream oss;
                    oss << value;
                    return oss.str();
                }
            }, variant);
        }

        template <typename T>
        explicit VariableValue(const std::string& name, const std::optional<T>& optional)
            : name(name), is_literal(is_valid_literal_name(name)), type(ValueType::OPTIONAL_VAL) {
            if (optional) {
                if constexpr (std::is_arithmetic_v<T>) {
                    string_value = std::to_string(*optional);
                } else if constexpr (std::is_same_v<T, std::string>) {
                    string_value = *optional;
                } else {
                    std::ostringstream oss;
                    oss << *optional;
                    string_value = oss.str();
                }
            } else {
                string_value = "nullopt";
            }
        }

#       endif

        explicit VariableValue(const std::string& name, void* ptr)
            : name(name), is_literal(is_valid_literal_name(name)), type(ValueType::POINTER_VAL) {
            std::ostringstream oss;
            oss << ptr;
            string_value = oss.str();
        }

        template <typename T>
        explicit VariableValue(const std::string& name, const std::shared_ptr<T>& ptr)
            : name(name), is_literal(is_valid_literal_name(name)), type(ValueType::SMART_POINTER_VAL) {
            std::ostringstream oss;
            if (ptr) oss << "shared_ptr@" << ptr.get();
            else oss << "nullptr";
            string_value = oss.str();
        }

        template <typename T>
        explicit VariableValue(const std::string& name, const std::unique_ptr<T>& ptr)
            : name(name), is_literal(is_valid_literal_name(name)), type(ValueType::SMART_POINTER_VAL) {
            std::ostringstream oss;
            if (ptr) oss << "unique_ptr@" << ptr.get();
            else oss << "nullptr";
            string_value = oss.str();
        }

        /// \brief Copy constructor.
        VariableValue(const VariableValue& other)
            : name(other.name), is_literal(other.is_literal), type(other.type),
              string_value(other.string_value),
              error_code_value(other.error_code_value) {
            if (is_pod_type(type)) {
                pod_value = other.pod_value;
            }
        }

        /// \brief Assignment operator.
        VariableValue& operator=(const VariableValue& other) {
            if (this == &other) return *this; // Self-assignment check.

            name = other.name;
            is_literal = other.is_literal;
            type = other.type;
            string_value = other.string_value;
            error_code_value = other.error_code_value;

            if (is_pod_type(type)) {
                pod_value = other.pod_value;
            }

            return *this;
        }

        /// \brief Destructor.
        ~VariableValue() = default;

        /// \brief Method to get the value as a string.
        /// \return String representation of the value.
        std::string to_string() const {
            switch (type) {
                case ValueType::INT8_VAL:    return std::to_string(pod_value.int8_value);
                case ValueType::UINT8_VAL:   return std::to_string(pod_value.uint8_value);
                case ValueType::INT16_VAL:   return std::to_string(pod_value.int16_value);
                case ValueType::UINT16_VAL:  return std::to_string(pod_value.uint16_value);
                case ValueType::INT32_VAL:   return std::to_string(pod_value.int32_value);
                case ValueType::UINT32_VAL:  return std::to_string(pod_value.uint32_value);
                case ValueType::INT64_VAL:   return std::to_string(pod_value.int64_value);
                case ValueType::UINT64_VAL:  return std::to_string(pod_value.uint64_value);
                case ValueType::BOOL_VAL:    return pod_value.bool_value ? "true" : "false";
                case ValueType::CHAR_VAL:    return std::string(1, pod_value.char_value);
                case ValueType::FLOAT_VAL:   return std::to_string(pod_value.float_value);
                case ValueType::DOUBLE_VAL:  return std::to_string(pod_value.double_value);
                case ValueType::LONG_DOUBLE_VAL: return std::to_string(static_cast<double>(pod_value.long_double_value));
                case ValueType::STRING_VAL:
                case ValueType::EXCEPTION_VAL:
                case ValueType::ENUM_VAL:
                case ValueType::PATH_VAL:
                case ValueType::DURATION_VAL:
                case ValueType::TIME_POINT_VAL:
                case ValueType::POINTER_VAL:
                case ValueType::SMART_POINTER_VAL:
                case ValueType::VARIANT_VAL:
                case ValueType::OPTIONAL_VAL:
                    return string_value;
                case ValueType::ERROR_CODE_VAL:
                    return error_code_value.message() + " (" + std::to_string(error_code_value.value()) + ")";
                default: break;
            }
            return "unknown";
        }

        /// \brief Method to get the value as a formatted string.
        /// \param fmt The format string.
        /// \return Formatted string representation of the value.
        std::string to_string(const char* fmt) const {
            switch (type) {
                case ValueType::INT8_VAL:    return logit::format(fmt, pod_value.int8_value);
                case ValueType::UINT8_VAL:   return format(fmt, pod_value.uint8_value);
                case ValueType::INT16_VAL:   return format(fmt, pod_value.int16_value);
                case ValueType::UINT16_VAL:  return format(fmt, pod_value.uint16_value);
                case ValueType::INT32_VAL:   return format(fmt, pod_value.int32_value);
                case ValueType::UINT32_VAL:  return format(fmt, pod_value.uint32_value);
                case ValueType::INT64_VAL:   return format(fmt, pod_value.int64_value);
                case ValueType::UINT64_VAL:  return format(fmt, pod_value.uint64_value);
                case ValueType::BOOL_VAL:    return format(fmt, pod_value.bool_value);
                case ValueType::CHAR_VAL:    return format(fmt, pod_value.char_value);
                case ValueType::FLOAT_VAL:   return format(fmt, pod_value.float_value);
                case ValueType::DOUBLE_VAL:  return format(fmt, pod_value.double_value);
                case ValueType::LONG_DOUBLE_VAL: return format(fmt, static_cast<double>(pod_value.long_double_value));
                case ValueType::STRING_VAL:
                case ValueType::EXCEPTION_VAL:
                case ValueType::ENUM_VAL:
                case ValueType::PATH_VAL:
                case ValueType::DURATION_VAL:
                case ValueType::TIME_POINT_VAL:
                case ValueType::POINTER_VAL:
                case ValueType::SMART_POINTER_VAL:
                case ValueType::VARIANT_VAL:
                case ValueType::OPTIONAL_VAL:
                    return format(fmt, string_value.c_str());
                case ValueType::ERROR_CODE_VAL:
                    return format(fmt, error_code_value.message().c_str(), error_code_value.value());
                default: break;
            }
            return "unknown";
        }

    private:
        /// \brief Helper function to check if a name is a valid literal.
        /// \param name The name to check.
        /// \return True if valid, false otherwise.
        static bool is_valid_literal_name(const std::string& name) {
            if (name.empty()) return false;
            return !std::isdigit(static_cast<unsigned char>(name[0]));
        }

        /// \brief Helper function to determine if a ValueType represents a POD type.
        /// \param type The ValueType to check.
        /// \return True if the type is POD, false otherwise.
        static bool is_pod_type(ValueType type) {
            switch (type) {
                case ValueType::INT8_VAL:
                case ValueType::UINT8_VAL:
                case ValueType::INT16_VAL:
                case ValueType::UINT16_VAL:
                case ValueType::INT32_VAL:
                case ValueType::UINT32_VAL:
                case ValueType::INT64_VAL:
                case ValueType::UINT64_VAL:
                case ValueType::BOOL_VAL:
                case ValueType::CHAR_VAL:
                case ValueType::FLOAT_VAL:
                case ValueType::DOUBLE_VAL:
                case ValueType::LONG_DOUBLE_VAL:
                    return true;
                default:
                    return false;
            }
        }

        /// \brief Helper function to get the unit of the duration.
        /// \tparam Period The period type of the duration.
        /// \return A string representing the unit of the duration.
        template <typename Period>
        static std::string duration_units() {
            if (std::is_same<Period, std::ratio<1>>::value) {
                return "s"; // seconds
            } else if (std::is_same<Period, std::milli>::value) {
                return "ms"; // milliseconds
            } else if (std::is_same<Period, std::micro>::value) {
                return "us"; // microseconds
            } else if (std::is_same<Period, std::nano>::value) {
                return "ns"; // nanoseconds
            } else if (std::is_same<Period, std::ratio<60>>::value) {
                return "min"; // minutes
            } else if (std::is_same<Period, std::ratio<3600>>::value) {
                return "h"; // hours
            } else {
                return "custom"; // Custom units
            }
        }
    }; // VariableValue

} // namespace logit

#endif // _LOGIT_VARIABLE_VALUE_HPP_INCLUDED
