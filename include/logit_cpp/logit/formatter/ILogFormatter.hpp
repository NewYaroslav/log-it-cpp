#pragma once
#ifndef _LOGIT_ILOG_FORMATTER_HPP_INCLUDED
#define _LOGIT_ILOG_FORMATTER_HPP_INCLUDED

/// \file ILogFormatter.hpp
/// \brief Defines the interface for log formatters used in the logging system.

#include <string>
#include <cstdint>
#include "LogItConfig.hpp"
#include "logit/enums.hpp"
#include "logit/utils/format.hpp"
#include "logit/utils/VariableValue.hpp"
#include "logit/utils/LogRecord.hpp"

namespace logit {

    /// \interface ILogFormatter
    /// \brief Interface for formatting log records.
    ///
    /// The `ILogFormatter` class defines an interface that any log formatter must implement.
    /// It provides a pure virtual function for formatting a log record into a string.
    class ILogFormatter {
    public:
        virtual ~ILogFormatter() = default;

        /// \brief Sets the timestamp offset for log formatting.
        ///
        /// This function allows setting a timezone offset in milliseconds, which will be used
        /// for adjusting timestamps in formatted log messages.
        ///
        /// \param offset_ms Timezone offset in milliseconds.
        virtual void set_timestamp_offset(int64_t offset_ms) = 0;

        /// \brief Formats a log record into a string.
        ///
        /// This pure virtual function must be implemented by any class deriving from `ILogFormatter`.
        /// The implementation should format the log record into a human-readable or machine-readable string.
        ///
        /// \param record The log record to be formatted.
        /// \return A string representing the formatted log message.
        virtual std::string format(const LogRecord& record) const = 0;
    }; // ILogFormatter

}; // namespace logit

#endif // _LOGIT_ILOG_FORMATTER_HPP_INCLUDED
