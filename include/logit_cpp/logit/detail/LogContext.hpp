#pragma once
#ifndef _LOGIT_LOG_CONTEXT_HPP_INCLUDED
#define _LOGIT_LOG_CONTEXT_HPP_INCLUDED

/// \file LogContext.hpp
/// \brief Thread-local Mapped Diagnostic Context (MDC) and Nested Diagnostic Context (NDC).

#include <map>
#include <memory>
#include <string>
#include <vector>

#ifndef LOGIT_HAS_FEATURE
#if defined(__has_feature)
#define LOGIT_HAS_FEATURE(x) __has_feature(x)
#else
#define LOGIT_HAS_FEATURE(x) 0
#endif
#endif

#ifndef LOGIT_DISABLE_THREAD_LOCAL_DESTRUCTORS
#if defined(__SANITIZE_ADDRESS__) || LOGIT_HAS_FEATURE(address_sanitizer)
#define LOGIT_DISABLE_THREAD_LOCAL_DESTRUCTORS 0
#else
#define LOGIT_DISABLE_THREAD_LOCAL_DESTRUCTORS 1
#endif
#endif

namespace logit {

#ifdef LOGIT_WITH_CONTEXT

    /// \struct LogContextSnapshot
    /// \brief Immutable MDC/NDC snapshot captured by a log record.
    struct LogContextSnapshot {
        std::map<std::string, std::string> mdc; ///< Mapped Diagnostic Context.
        std::vector<std::string> ndc;           ///< Nested Diagnostic Context.
    };

    namespace detail {

#           if LOGIT_DISABLE_THREAD_LOCAL_DESTRUCTORS

        inline std::map<std::string, std::string>*& mdc_map_storage() {
            static thread_local std::map<std::string, std::string>* instance = nullptr;
            return instance;
        }

        inline std::vector<std::string>*& ndc_stack_storage() {
            static thread_local std::vector<std::string>* instance = nullptr;
            return instance;
        }

#           else

        inline std::unique_ptr<std::map<std::string, std::string> >& mdc_map_storage() {
            static thread_local std::unique_ptr<std::map<std::string, std::string> > instance;
            return instance;
        }

        inline std::unique_ptr<std::vector<std::string> >& ndc_stack_storage() {
            static thread_local std::unique_ptr<std::vector<std::string> > instance;
            return instance;
        }

#           endif

        inline std::map<std::string, std::string>& mutable_mdc_map() {
#           if LOGIT_DISABLE_THREAD_LOCAL_DESTRUCTORS
            if (!mdc_map_storage()) {
                mdc_map_storage() = new std::map<std::string, std::string>();
            }
            return *mdc_map_storage();
#           else
            if (!mdc_map_storage()) {
                mdc_map_storage().reset(new std::map<std::string, std::string>());
            }
            return *mdc_map_storage();
#           endif
        }

        inline std::vector<std::string>& mutable_ndc_stack() {
#           if LOGIT_DISABLE_THREAD_LOCAL_DESTRUCTORS
            if (!ndc_stack_storage()) {
                ndc_stack_storage() = new std::vector<std::string>();
            }
            return *ndc_stack_storage();
#           else
            if (!ndc_stack_storage()) {
                ndc_stack_storage().reset(new std::vector<std::string>());
            }
            return *ndc_stack_storage();
#           endif
        }

        inline const std::map<std::string, std::string>* mdc_map_if_exists() {
#           if LOGIT_DISABLE_THREAD_LOCAL_DESTRUCTORS
            return mdc_map_storage();
#           else
            return mdc_map_storage().get();
#           endif
        }

        inline const std::vector<std::string>* ndc_stack_if_exists() {
#           if LOGIT_DISABLE_THREAD_LOCAL_DESTRUCTORS
            return ndc_stack_storage();
#           else
            return ndc_stack_storage().get();
#           endif
        }

    } // namespace detail

    /// \brief Put a key-value pair into the MDC for the current thread.
    inline void mdc_put(const std::string& key, const std::string& value) {
        detail::mutable_mdc_map()[key] = value;
    }

    /// \brief Remove a key from the MDC for the current thread.
    inline void mdc_remove(const std::string& key) {
        std::map<std::string, std::string>* values =
            const_cast<std::map<std::string, std::string>*>(detail::mdc_map_if_exists());
        if (values != nullptr) {
            values->erase(key);
        }
    }

    /// \brief Clear all MDC entries for the current thread.
    inline void mdc_clear() {
        std::map<std::string, std::string>* values =
            const_cast<std::map<std::string, std::string>*>(detail::mdc_map_if_exists());
        if (values != nullptr) {
            values->clear();
        }
    }

    /// \brief Push a message onto the NDC stack for the current thread.
    inline void ndc_push(const std::string& message) {
        detail::mutable_ndc_stack().push_back(message);
    }

    /// \brief Pop the top message from the NDC stack for the current thread.
    inline void ndc_pop() {
        std::vector<std::string>* stack =
            const_cast<std::vector<std::string>*>(detail::ndc_stack_if_exists());
        if (stack != nullptr && !stack->empty()) {
            stack->pop_back();
        }
    }

    /// \brief Clear the NDC stack for the current thread.
    inline void ndc_clear() {
        std::vector<std::string>* stack =
            const_cast<std::vector<std::string>*>(detail::ndc_stack_if_exists());
        if (stack != nullptr) {
            stack->clear();
        }
    }

    /// \brief Captures current thread MDC/NDC if either context is non-empty.
    inline std::shared_ptr<const LogContextSnapshot> capture_log_context() {
        const std::map<std::string, std::string>* mdc = detail::mdc_map_if_exists();
        const std::vector<std::string>* ndc = detail::ndc_stack_if_exists();
        if ((mdc == nullptr || mdc->empty()) && (ndc == nullptr || ndc->empty())) {
            return std::shared_ptr<const LogContextSnapshot>();
        }

        std::shared_ptr<LogContextSnapshot> snapshot(new LogContextSnapshot());
        if (mdc != nullptr) {
            snapshot->mdc = *mdc;
        }
        if (ndc != nullptr) {
            snapshot->ndc = *ndc;
        }
        return snapshot;
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

#endif // LOGIT_WITH_CONTEXT

} // namespace logit

#endif // _LOGIT_LOG_CONTEXT_HPP_INCLUDED
