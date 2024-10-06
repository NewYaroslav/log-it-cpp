#pragma once
#ifndef _LOGIT_ILOG_FORMATTER_HPP_INCLUDED
#define _LOGIT_ILOG_FORMATTER_HPP_INCLUDED
/// \file ILogFormatter.hpp
/// \brief Defines the interface for log formatters used in the logging system.

#include "../Utils/LogRecord.hpp"

namespace logit {

	/// \class ILogFormatter
	/// \brief Interface for formatting log records.
	///
	/// The `ILogFormatter` class defines an interface that any log formatter must implement.
	/// It provides a pure virtual function for formatting a log record into a string.
	class ILogFormatter {
	public:
		virtual ~ILogFormatter() = default;

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
