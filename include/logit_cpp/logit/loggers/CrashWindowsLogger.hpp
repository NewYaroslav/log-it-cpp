#pragma once
#ifndef LOGIT_CRASH_WINDOWS_LOGGER_HPP_INCLUDED
#define LOGIT_CRASH_WINDOWS_LOGGER_HPP_INCLUDED

/// \\file CrashWindowsLogger.hpp
/// \\brief Windows crash logger persisting the last messages to a file handle.

#include "ILogger.hpp"

#include <array>
#include <atomic>
#include <cstddef>
#include <cstring>
#include <string>

#if defined(_WIN32)
#    ifndef NOMINMAX
#        define NOMINMAX
#    endif
#    ifndef WIN32_LEAN_AND_MEAN
#        define WIN32_LEAN_AND_MEAN
#    endif
#    include <windows.h>
#    include <process.h>
#    include <cstdlib>
#endif

namespace logit {

#if defined(_WIN32)

    /// \\class CrashWindowsLogger
    /// \\ingroup LogBackends
    /// \\brief Maintains an in-memory ring buffer of recent messages and dumps it on crashes.
    class CrashWindowsLogger : public ILogger {
    public:
        /// \\brief Maximum storage reserved for the ring buffer.
        static constexpr std::size_t kMaxBufferSize = 64 * 1024; // 64 KiB

        /// \\brief Default amount of bytes stored from the most recent messages.
        static constexpr std::size_t kDefaultBufferSize = kMaxBufferSize;

        /// \\struct Config
        /// \\brief Runtime configuration for the crash logger.
        struct Config {
            std::string log_path = "crash.log";          ///< Path to the crash log file.
            std::size_t buffer_size = kDefaultBufferSize; ///< Bytes kept in the in-memory buffer.
        };

        /// \\brief Construct with default configuration.
        CrashWindowsLogger() : CrashWindowsLogger(Config()) {}

        /// \\brief Construct with explicit configuration.
        /// \\param config Configuration parameters.
        explicit CrashWindowsLogger(const Config& config)
                : m_log_path(config.log_path),
                  m_capacity(config.buffer_size > kMaxBufferSize ? kMaxBufferSize : config.buffer_size) {
            if (m_capacity == 0) {
                m_capacity = 1;
            }
            m_file = ::CreateFileA(
                    m_log_path.c_str(),
                    FILE_APPEND_DATA,
                    FILE_SHARE_READ | FILE_SHARE_WRITE,
                    nullptr,
                    OPEN_ALWAYS,
                    FILE_ATTRIBUTE_NORMAL,
                    nullptr);
            active_logger().store(this, std::memory_order_release);
        }

        CrashWindowsLogger(const CrashWindowsLogger&) = delete;
        CrashWindowsLogger& operator=(const CrashWindowsLogger&) = delete;

        /// \\brief Close the crash log handle on destruction.
        ~CrashWindowsLogger() override {
            CrashWindowsLogger* expected = this;
            active_logger().compare_exchange_strong(
                    expected, nullptr, std::memory_order_release, std::memory_order_relaxed);
            if (m_file != INVALID_HANDLE_VALUE) {
                ::CloseHandle(m_file);
            }
        }

        /// \\brief Store the message in the lock-free ring buffer.
        /// \\param record Log metadata.
        /// \\param message Formatted message text.
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

        /// \\brief Return path to the crash log when requested.
        std::string get_string_param(const LoggerParam& param) const override {
            switch (param) {
            case LoggerParam::LastFilePath:
                return m_log_path;
            default:
                break;
            }
            return std::string();
        }

        /// \\brief Integer parameters are not tracked.
        int64_t get_int_param(const LoggerParam& param) const override {
            (void)param;
            return 0;
        }

        /// \\brief Floating-point parameters are not tracked.
        double get_float_param(const LoggerParam& param) const override {
            (void)param;
            return 0.0;
        }

        /// \\brief Set minimal log level.
        void set_log_level(LogLevel level) override {
            m_log_level.store(static_cast<int>(level), std::memory_order_relaxed);
        }

        /// \\brief Get minimal log level.
        LogLevel get_log_level() const override {
            return static_cast<LogLevel>(m_log_level.load(std::memory_order_relaxed));
        }

        /// \\brief Crash logger operates synchronously.
        void wait() override {}

        /// \\brief Install unhandled exception handler dumping the buffer before exiting.
        static void install_exception_handler() {
            ::SetUnhandledExceptionFilter(&CrashWindowsLogger::exception_filter);
        }

    private:
        static LONG WINAPI exception_filter(EXCEPTION_POINTERS* exception_info) {
            CrashWindowsLogger* logger = active_logger().load(std::memory_order_acquire);
            if (logger != nullptr) {
                DWORD code = 0;
                if (exception_info != nullptr && exception_info->ExceptionRecord != nullptr) {
                    code = exception_info->ExceptionRecord->ExceptionCode;
                }
                logger->write_snapshot(code);
            }
            _exit(EXIT_FAILURE);
            return EXCEPTION_EXECUTE_HANDLER;
        }

        void write_snapshot(DWORD exception_code) const noexcept {
            if (m_file == INVALID_HANDLE_VALUE) {
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

            write_marker(exception_code);
            ::FlushFileBuffers(m_file);
        }

        void safe_write(const char* data, std::size_t size) const noexcept {
            if (m_file == INVALID_HANDLE_VALUE) {
                return;
            }

            while (size > 0) {
                DWORD to_write = static_cast<DWORD>(size);
                DWORD written = 0;
                if (!::WriteFile(m_file, data, to_write, &written, nullptr) || written == 0) {
                    break;
                }
                data += written;
                size -= written;
            }
        }

        void write_marker(DWORD exception_code) const noexcept {
            char marker[64];
            std::size_t idx = 0;
            const char prefix[] = "\n== CRASH EXCEPTION 0x";
            const char suffix[] = " ==\n";
            for (std::size_t i = 0; i < sizeof(prefix) - 1 && idx < sizeof(marker); ++i) {
                marker[idx++] = prefix[i];
            }

            unsigned long code = exception_code;
            char digits[16];
            std::size_t digit_count = 0;
            if (code == 0) {
                digits[digit_count++] = '0';
            } else {
                while (code > 0 && digit_count < sizeof(digits)) {
                    unsigned long value = code & 0xF;
                    digits[digit_count++] = static_cast<char>((value < 10) ? ('0' + value) : ('A' + (value - 10)));
                    code >>= 4;
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
        HANDLE m_file{INVALID_HANDLE_VALUE};
        std::size_t m_capacity{0};
        std::atomic<std::size_t> m_next_offset{0};
        std::atomic<std::size_t> m_committed{0};
        std::atomic<int> m_log_level{static_cast<int>(LogLevel::LOG_LVL_TRACE)};
        std::array<char, kMaxBufferSize> m_buffer{};

        static std::atomic<CrashWindowsLogger*>& active_logger() noexcept {
            static std::atomic<CrashWindowsLogger*> instance(nullptr);
            return instance;
        }
    };

#else // Stub for non-Windows systems

    /// \\class CrashWindowsLogger
    /// \\brief Stub implementation on non-Windows platforms.
    class CrashWindowsLogger : public ILogger {
    public:
        static constexpr std::size_t kMaxBufferSize = 0;
        static constexpr std::size_t kDefaultBufferSize = 0;

        struct Config {
            std::string log_path{};
            std::size_t buffer_size = 0;
        };

        CrashWindowsLogger() = default;
        explicit CrashWindowsLogger(const Config&) {}

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

        static void install_exception_handler() {}
    };

#endif

} // namespace logit

#endif // LOGIT_CRASH_WINDOWS_LOGGER_HPP_INCLUDED
