#pragma once
#ifndef _LOGIT_TEST_LOG_DEPTH_1_HPP_INCLUDED
#define _LOGIT_TEST_LOG_DEPTH_1_HPP_INCLUDED

/// \file test_log_depth_1.hpp
/// \brief Test file for verifying log path shortening at depth level 1.

namespace logit {

    /// \brief Logs test messages to verify path shortening at depth level 1.
    void test_log_depth_1() {
        LOGIT_TRACE0();
        LOGIT_TRACE("This is an informational message");
    }

}; // namespace logit

#endif // _LOGIT_TEST_LOG_DEPTH_1_HPP_INCLUDED
