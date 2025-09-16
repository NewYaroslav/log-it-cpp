#pragma once
#ifndef _LOGIT_TEST_LOG_DEPTH_2_HPP_INCLUDED
#define _LOGIT_TEST_LOG_DEPTH_2_HPP_INCLUDED

/// \file test_log_depth_2.hpp
/// \brief Test file for verifying log path shortening at depth level 2.

namespace logit {

    /// \brief Logs test messages to verify path shortening at depth level 2.
    void test_log_depth_2() {
        LOGIT_TRACE0();
        LOGIT_TRACE("This is an informational message");
    }

}; // namespace logit

#endif // _LOGIT_TEST_LOG_DEPTH_2_HPP_INCLUDED

