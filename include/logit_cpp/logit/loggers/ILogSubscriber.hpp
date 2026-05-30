#pragma once
#ifndef _LOGIT_ILOG_SUBSCRIBER_HPP_INCLUDED
#define _LOGIT_ILOG_SUBSCRIBER_HPP_INCLUDED

/// \file ILogSubscriber.hpp
/// \brief Optional live-subscription interface for log backends that can push newly written records.

#include "ILogReader.hpp"
#include <cstdint>
#include <functional>

namespace logit {

    /// \class ILogSubscriber
    /// \brief Optional interface for backends that can notify callers when a record is successfully written.
    ///
    /// Callbacks receive a \ref LogRecordView containing the persisted preview and metadata.
    /// They are guaranteed to be invoked only after the record has been committed to storage.
    /// Implementations must not hold internal locks while invoking callbacks.
    class ILogSubscriber {
    public:
        using Callback = std::function<void(const LogRecordView&)>;

        virtual ~ILogSubscriber() = default;

        /// \brief Registers a callback to be invoked after each successfully written record.
        /// \param callback Function called with the written LogRecordView.
        /// \return Stable callback id that can be passed to remove_log_callback.
        virtual uint64_t add_log_callback(Callback callback) = 0;

        /// \brief Unregisters a previously added callback.
        /// \param callback_id Id returned by add_log_callback.
        /// \return True if the callback existed and was removed.
        virtual bool remove_log_callback(uint64_t callback_id) = 0;
    };

} // namespace logit

#endif // _LOGIT_ILOG_SUBSCRIBER_HPP_INCLUDED
