#pragma once
#ifndef _LOGIT_LOG_CONTEXT_HPP_INCLUDED
#define _LOGIT_LOG_CONTEXT_HPP_INCLUDED

/// \file LogContext.hpp
/// \brief Thread-local Mapped Diagnostic Context (MDC) and Nested Diagnostic Context (NDC).

#include <map>
#include <string>
#include <vector>

#ifndef LOGIT_DISABLE_THREAD_LOCAL_DESTRUCTORS
#define LOGIT_DISABLE_THREAD_LOCAL_DESTRUCTORS 1
#endif

namespace logit {

    namespace detail {

        inline std::map<std::string, std::string>& mdc_map() {
#           if LOGIT_DISABLE_THREAD_LOCAL_DESTRUCTORS
            thread_local auto* instance = new std::map<std::string, std::string>();
            return *instance;
#           else
            thread_local std::map<std::string, std::string> instance;
            return instance;
#           endif
        }

        inline std::vector<std::string>& ndc_stack() {
#           if LOGIT_DISABLE_THREAD_LOCAL_DESTRUCTORS
            thread_local auto* instance = new std::vector<std::string>();
            return *instance;
#           else
            thread_local std::vector<std::string> instance;
            return instance;
#           endif
        }

    } // namespace detail

    /// \brief Put a key-value pair into the MDC for the current thread.
    inline void mdc_put(const std::string& key, const std::string& value) {
        detail::mdc_map()[key] = value;
    }

    /// \brief Remove a key from the MDC for the current thread.
    inline void mdc_remove(const std::string& key) {
        detail::mdc_map().erase(key);
    }

    /// \brief Clear all MDC entries for the current thread.
    inline void mdc_clear() {
        detail::mdc_map().clear();
    }

    /// \brief Push a message onto the NDC stack for the current thread.
    inline void ndc_push(const std::string& message) {
        detail::ndc_stack().push_back(message);
    }

    /// \brief Pop the top message from the NDC stack for the current thread.
    inline void ndc_pop() {
        std::vector<std::string>& stack = detail::ndc_stack();
        if (!stack.empty()) {
            stack.pop_back();
        }
    }

    /// \brief Clear the NDC stack for the current thread.
    inline void ndc_clear() {
        detail::ndc_stack().clear();
    }

    /// \class NdcGuard
    /// \brief RAII guard that pushes a message on construction and pops on destruction.
    ///
    /// Intended for strictly stack-scoped usage. If manual ndc_push()/ndc_pop()
    /// calls interleave the guard lifetime, the guard still pops once.
    class NdcGuard {
    public:
        explicit NdcGuard(const std::string& message) {
            ndc_push(message);
        }

        ~NdcGuard() {
            ndc_pop();
        }

        NdcGuard(const NdcGuard&) = delete;
        NdcGuard& operator=(const NdcGuard&) = delete;
        NdcGuard(NdcGuard&&) = delete;
        NdcGuard& operator=(NdcGuard&&) = delete;
    };

} // namespace logit

#endif // _LOGIT_LOG_CONTEXT_HPP_INCLUDED
