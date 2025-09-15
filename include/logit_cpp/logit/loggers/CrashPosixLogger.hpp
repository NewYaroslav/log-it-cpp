#pragma once
#ifndef LOGIT_CRASH_POSIX_LOGGER_HPP_INCLUDED
#define LOGIT_CRASH_POSIX_LOGGER_HPP_INCLUDED

/// \file CrashPosixLogger.hpp
/// \brief POSIX crash logger persisting the last messages to a file descriptor.

#include "ILogger.hpp"
#include "../utils.hpp"

#include <atomic>
#include <array>
#include <cstddef>
#include <cstring>
#include <string>

#if defined(__unix__) || defined(__APPLE__) || defined(__linux__)
#include <cerrno>
#include <fcntl.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#endif

namespace logit {

#if defined(__unix__) || defined(__APPLE__) || defined(__linux__)

    /// \class CrashPosixLogger
    /// \ingroup LogBackends
    /// \brief Maintains an in-memory ring buffer of recent messages and dumps it on crashes.
    class CrashPosixLogger : public ILogger {
    public:
        /// \brief Maximum storage reserved for the ring buffer.
        static constexpr std::size_t kMaxBufferSize = 64 * 1024; // 64 KiB

        /// \brief Default amount of bytes stored from the most recent messages.
        static constexpr std::size_t kDefaultBufferSize = kMaxBufferSize;

        /// \struct Config
        /// \brief Runtime configuration for the crash logger.
        struct Config {
            std::string log_path = "crash.log";          ///< Path to the crash log file.
            std::size_t buffer_size = kDefaultBufferSize; ///< Bytes kept in the in-memory buffer.
        };

        /// \brief Construct with default configuration.
        CrashPosixLogger() : CrashPosixLogger(Config()) {}

        /// \brief Construct with explicit configuration.
        /// \param config Configuration parameters.
        explicit CrashPosixLogger(const Config& config) :
                m_log_path(config.log_path),
                m_capacity(config.buffer_size > kMaxBufferSize ? kMaxBufferSize : config.buffer_size) {
            if (m_capacity == 0) {
                m_capacity = 1;
            }
            int flags = O_WRONLY | O_CREAT | O_APPEND;
#ifdef O_CLOEXEC
            flags |= O_CLOEXEC;
#endif
            m_fd = ::open(m_log_path.c_str(), flags, static_cast<mode_t>(S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH));
#ifdef O_CLOEXEC
            if (m_fd < 0 && (flags & O_CLOEXEC) != 0) {
                flags &= ~O_CLOEXEC;
                m_fd = ::open(m_log_path.c_str(), flags, static_cast<mode_t>(S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH));
            }
#endif
            s_active_logger.store(this, std::memory_order_release);
        }

        CrashPosixLogger(const CrashPosixLogger&) = delete;
        CrashPosixLogger& operator=(const CrashPosixLogger&) = delete;

        /// \brief Close the crash log descriptor on destruction.
        ~CrashPosixLogger() override {
            CrashPosixLogger* expected = this;
            s_active_logger.compare_exchange_strong(
                    expected, nullptr, std::memory_order_release, std::memory_order_relaxed);
            if (m_fd >= 0) {
                ::close(m_fd);
            }
        }

        /// \brief Store the message in the lock-free ring buffer.
        /// \param record Log metadata.
        /// \param message Formatted message text.
        void log(const LogRecord& record, const std::string& message) override {
            if (static_cast<int>(record.log_level) < m_log_level.load(std::memory_order_relaxed)) {
                return;
            }

            const std::size_t capacity = m_capacity;
            if (capacity == 0) {
                return;
            }

            const char* data = message.data();
            std::size_t length = message.size();

            std::size_t copy_len = length;
            if (copy_len >= capacity) {
                if (capacity > 1) {
                    data += length - (capacity - 1);
                    copy_len = capacity - 1;
                } else {
                    data += length;
                    copy_len = 0;
                }
            }

            const std::size_t total = copy_len + 1; // extra byte for newline marker
            const std::size_t start = m_next_offset.fetch_add(total, std::memory_order_acq_rel);
            const std::size_t index = capacity ? (start % capacity) : 0;

            if (copy_len > 0) {
                std::size_t first = capacity - index;
                if (first > copy_len) {
                    first = copy_len;
                }
                std::memcpy(m_buffer.data() + index, data, first);
                const std::size_t remaining = copy_len - first;
                if (remaining > 0) {
                    std::memcpy(m_buffer.data(), data + first, remaining);
                }
            }

            const std::size_t newline_index = capacity ? ((index + copy_len) % capacity) : 0;
            m_buffer[newline_index] = '\n';

            const std::size_t desired = start + total;
            std::size_t expected = m_committed.load(std::memory_order_relaxed);
            while (expected < desired &&
                   !m_committed.compare_exchange_weak(
                           expected, desired, std::memory_order_release, std::memory_order_relaxed)) {
            }
        }

        /// \brief Return path to the crash log when requested.
        std::string get_string_param(const LoggerParam& param) const override {
            switch (param) {
            case LoggerParam::LastFilePath:
                return m_log_path;
            default:
                break;
            }
            return std::string();
        }

        /// \brief Integer parameters are not tracked.
        int64_t get_int_param(const LoggerParam& param) const override {
            (void)param;
            return 0;
        }

        /// \brief Floating-point parameters are not tracked.
        double get_float_param(const LoggerParam& param) const override {
            (void)param;
            return 0.0;
        }

        /// \brief Set minimal log level.
        void set_log_level(LogLevel level) override {
            m_log_level.store(static_cast<int>(level), std::memory_order_relaxed);
        }

        /// \brief Get minimal log level.
        LogLevel get_log_level() const override {
            return static_cast<LogLevel>(m_log_level.load(std::memory_order_relaxed));
        }

        /// \brief Crash logger operates synchronously.
        void wait() override {}

        /// \brief Install sigaction handler dumping the buffer before exiting.
        /// \param signo Signal number.
        static void install_signal_handler(int signo) {
            struct sigaction action;
            std::memset(&action, 0, sizeof(action));
            action.sa_sigaction = &CrashPosixLogger::signal_handler;
            sigemptyset(&action.sa_mask);
            action.sa_flags = SA_SIGINFO | SA_RESTART;
            sigaction(signo, &action, nullptr);
        }

    private:
        static void signal_handler(int signo, siginfo_t* info, void* context) {
            (void)info;
            (void)context;
            CrashPosixLogger* logger = s_active_logger.load(std::memory_order_acquire);
            if (logger != nullptr) {
                logger->write_snapshot(signo);
            }
            _exit(128 + signo);
        }

        void write_snapshot(int signo) const noexcept {
            if (m_fd < 0) {
                return;
            }

            const std::size_t capacity = m_capacity;
            const std::size_t committed = m_committed.load(std::memory_order_acquire);
            std::size_t available = committed;
            if (available > capacity) {
                available = capacity;
            }
            const std::size_t start_offset = committed - available;
            const std::size_t start_index = capacity ? (start_offset % capacity) : 0;

            if (available > 0) {
                std::size_t chunk = capacity - start_index;
                if (chunk > available) {
                    chunk = available;
                }
                safe_write(m_buffer.data() + start_index, chunk);
                const std::size_t remaining = available - chunk;
                if (remaining > 0) {
                    safe_write(m_buffer.data(), remaining);
                }
            }

            write_marker(signo);
        }

        void safe_write(const char* data, std::size_t size) const noexcept {
            while (size > 0) {
                const ssize_t written = ::write(m_fd, data, size);
                if (written <= 0) {
                    if (written < 0 && errno == EINTR) {
                        continue;
                    }
                    break;
                }
                const std::size_t advance = static_cast<std::size_t>(written);
                data += advance;
                size -= advance;
            }
        }

        void write_marker(int signo) const noexcept {
            char marker[64];
            std::size_t idx = 0;
            const char prefix[] = "\n== CRASH SIGNAL ";
            const char suffix[] = " ==\n";
            for (std::size_t i = 0; i < sizeof(prefix) - 1 && idx < sizeof(marker); ++i) {
                marker[idx++] = prefix[i];
            }

            int value = signo;
            if (value < 0) {
                if (idx < sizeof(marker)) {
                    marker[idx++] = '-';
                }
                value = -value;
            }

            char digits[32];
            std::size_t digit_count = 0;
            unsigned int uvalue = static_cast<unsigned int>(value);
            if (uvalue == 0) {
                digits[digit_count++] = '0';
            } else {
                while (uvalue > 0 && digit_count < sizeof(digits)) {
                    digits[digit_count++] = static_cast<char>('0' + (uvalue % 10));
                    uvalue /= 10;
                }
            }
            while (digit_count > 0 && idx < sizeof(marker)) {
                marker[idx++] = digits[--digit_count];
            }

            for (std::size_t i = 0; i < sizeof(suffix) - 1 && idx < sizeof(marker); ++i) {
                marker[idx++] = suffix[i];
            }

            safe_write(marker, idx);
        }

        std::string m_log_path;
        int m_fd{-1};
        std::size_t m_capacity{0};
        std::atomic<std::size_t> m_next_offset{0};
        std::atomic<std::size_t> m_committed{0};
        std::atomic<int> m_log_level{static_cast<int>(LogLevel::LOG_LVL_TRACE)};
        std::array<char, kMaxBufferSize> m_buffer{};

        inline static std::atomic<CrashPosixLogger*> s_active_logger{nullptr};
    };

#else // Stub for non-POSIX systems

    /// \class CrashPosixLogger
    /// \brief Stub implementation on non-POSIX platforms.
    class CrashPosixLogger : public ILogger {
    public:
        static constexpr std::size_t kMaxBufferSize = 0;
        static constexpr std::size_t kDefaultBufferSize = 0;

        struct Config {
            std::string log_path{};
            std::size_t buffer_size = 0;
        };

        CrashPosixLogger() = default;
        explicit CrashPosixLogger(const Config&) {}

        void log(const LogRecord& record, const std::string& message) override {
            (void)record;
            (void)message;
        }

        std::string get_string_param(const LoggerParam& param) const override {
            (void)param;
            return std::string();
        }

        int64_t get_int_param(const LoggerParam& param) const override {
            (void)param;
            return 0;
        }

        double get_float_param(const LoggerParam& param) const override {
            (void)param;
            return 0.0;
        }

        void set_log_level(LogLevel level) override {
            (void)level;
        }

        LogLevel get_log_level() const override {
            return LogLevel::LOG_LVL_TRACE;
        }

        void wait() override {}

        static void install_signal_handler(int signo) {
            (void)signo;
        }
    };

#endif

} // namespace logit

#endif // LOGIT_CRASH_POSIX_LOGGER_HPP_INCLUDED
