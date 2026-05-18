#pragma once
#ifndef _LOGIT_DETAIL_QUEUE_POLICY_HPP_INCLUDED
#define _LOGIT_DETAIL_QUEUE_POLICY_HPP_INCLUDED

namespace logit { namespace detail {

/// \brief Queue overflow handling policy used by TaskExecutor and SingleThreadExecutor.
enum class QueuePolicy {
    DropNewest, ///< Reject the incoming task when the queue is full.
    DropOldest, ///< Drop the oldest queued task.
    Block       ///< Producers wait until capacity is available.
};

}} // namespace logit::detail

#endif // _LOGIT_DETAIL_QUEUE_POLICY_HPP_INCLUDED
