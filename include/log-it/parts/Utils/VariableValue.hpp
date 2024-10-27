#pragma once
#ifndef _LOGIT_VARIABLE_VALUE_HPP_INCLUDED
#define _LOGIT_VARIABLE_VALUE_HPP_INCLUDED
/// \file VariableValue.hpp
/// \brief Structure for storing variables of various types.

#include "format.hpp"
#include <string>
#include <iostream>
#include <cstdint>
#include <type_traits>
#include <exception>

namespace logit {

    /// \struct VariableValue
    /// \brief Structure for storing values of various types, including enumerations.
    struct VariableValue {
        std::string name;       ///< Variable name.
        bool is_literal;        ///< Flag indicating if the variable is a literal.

        /// \enum ValueType
        /// \brief Enumeration of possible value types.
        enum class ValueType {
            INT8_VAL,
            UINT8_VAL,
            INT16_VAL,
            UINT16_VAL,
            INT32_VAL,
            UINT32_VAL,
            INT64_VAL,
            UINT64_VAL,
            BOOL_VAL,
            CHAR_VAL,
            FLOAT_VAL,
            DOUBLE_VAL,
            STRING_VAL,
            EXCEPTION_VAL,
            ENUM_VAL,
            UNKNOWN_VAL
        } type;

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
        } pod_value; ///< Union to store POD types.

        std::string string_value; ///< Variable to store string, exception messages, and enums.

        // Constructors for each type.
        VariableValue(const std::string& name, int8_t value)
            : name(name), is_literal(is_valid_literal_name(name)), type(ValueType::INT8_VAL) {
            pod_value.int8_value = value;
        }

        VariableValue(const std::string& name, uint8_t value)
            : name(name), is_literal(is_valid_literal_name(name)), type(ValueType::UINT8_VAL) {
            pod_value.uint8_value = value;
        }

        VariableValue(const std::string& name, int16_t value)
            : name(name), is_literal(is_valid_literal_name(name)), type(ValueType::INT16_VAL) {
            pod_value.int16_value = value;
        }

        VariableValue(const std::string& name, uint16_t value)
            : name(name), is_literal(is_valid_literal_name(name)), type(ValueType::UINT16_VAL) {
            pod_value.uint16_value = value;
        }

        VariableValue(const std::string& name, int32_t value)
            : name(name), is_literal(is_valid_literal_name(name)), type(ValueType::INT32_VAL) {
            pod_value.int32_value = value;
        }

        VariableValue(const std::string& name, uint32_t value)
            : name(name), is_literal(is_valid_literal_name(name)), type(ValueType::UINT32_VAL) {
            pod_value.uint32_value = value;
        }

        VariableValue(const std::string& name, int64_t value)
            : name(name), is_literal(is_valid_literal_name(name)), type(ValueType::INT64_VAL) {
            pod_value.int64_value = value;
        }

        VariableValue(const std::string& name, uint64_t value)
            : name(name), is_literal(is_valid_literal_name(name)), type(ValueType::UINT64_VAL) {
            pod_value.uint64_value = value;
        }

        VariableValue(const std::string& name, bool value)
            : name(name), is_literal(is_valid_literal_name(name)), type(ValueType::BOOL_VAL) {
            pod_value.bool_value = value;
        }

        VariableValue(const std::string& name, char value)
            : name(name), is_literal(is_valid_literal_name(name)), type(ValueType::CHAR_VAL) {
            pod_value.char_value = value;
        }

        VariableValue(const std::string& name, float value)
            : name(name), is_literal(is_valid_literal_name(name)), type(ValueType::FLOAT_VAL) {
            pod_value.float_value = value;
        }

        VariableValue(const std::string& name, double value)
            : name(name), is_literal(is_valid_literal_name(name)), type(ValueType::DOUBLE_VAL) {
            pod_value.double_value = value;
        }

        VariableValue(const std::string& name, const std::string& value)
            : name(name), is_literal(is_valid_literal_name(name)), type(ValueType::STRING_VAL), string_value(value) {
        }

        VariableValue(const std::string& name, const char* value)
            : name(name), is_literal(is_valid_literal_name(name)), type(ValueType::STRING_VAL), string_value(value) {
        }

        VariableValue(const std::string& name, const std::exception& ex)
            : name(name), is_literal(is_valid_literal_name(name)), type(ValueType::EXCEPTION_VAL), string_value(ex.what()) {
        }

        /// \brief Constructor for enumerations.
        /// \tparam EnumType The enumeration type.
        /// \param name The variable name.
        /// \param value The enumeration value.
        template <typename EnumType>
        VariableValue(const std::string& name, EnumType value,
            typename std::enable_if<std::is_enum<EnumType>::value>::type* = 0)
            : name(name), is_literal(is_valid_literal_name(name)), type(ValueType::ENUM_VAL), string_value(enum_to_string(value)) {
        }

        /// \brief Copy constructor.
        VariableValue(const VariableValue& other)
            : name(other.name), is_literal(other.is_literal), type(other.type), string_value(other.string_value) {
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
                case ValueType::STRING_VAL:
                case ValueType::EXCEPTION_VAL:
                case ValueType::ENUM_VAL:
                    return string_value;
                default:
                    return "unknown";
            }
        }

        /// \brief Method to get the value as a formatted string.
        /// \param fmt The format string.
        /// \return Formatted string representation of the value.
        std::string to_string(const char* fmt) const {
            switch (type) {
                case ValueType::INT8_VAL:    return format(fmt, pod_value.int8_value);
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
                case ValueType::STRING_VAL:
                case ValueType::EXCEPTION_VAL:
                case ValueType::ENUM_VAL:
                    return format(fmt, string_value.c_str());
                default:
                    return "unknown";
            }
        }

    private:
        /// \brief Helper function to check if a name is a valid literal.
        /// \param name The name to check.
        /// \return True if valid, false otherwise.
        static bool is_valid_literal_name(const std::string& name) {
            if (name.empty()) return false;
            return !std::isdigit(static_cast<unsigned char>(name[0]));
        }

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
                    return true;
                default:
                    return false;
            }
        }
    };

} // namespace logit

#endif // _LOGIT_VARIABLE_VALUE_HPP_INCLUDED
