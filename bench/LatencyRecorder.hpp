#pragma once

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <vector>

namespace logit_bench {

class LatencyRecorder {
public:
    struct Token {
        std::uint64_t slot = invalid_slot();
        std::uint64_t t0_ns = 0;
        bool active = false;
    };

    struct Summary {
        std::uint64_t p50_ns = 0;
        std::uint64_t p99_ns = 0;
        std::uint64_t p999_ns = 0;
    };

    explicit LatencyRecorder(std::size_t total)
        : m_values(total),
          m_expected(total),
          m_next_slot(0) {}

    Token begin(bool record) {
        Token token;
        token.active = record;
        token.t0_ns = now();
        if (record) {
            const auto slot = m_next_slot.fetch_add(1, std::memory_order_relaxed);
            if (slot >= m_expected) {
                throw std::out_of_range("LatencyRecorder capacity exceeded");
            }
            token.slot = static_cast<std::uint64_t>(slot);
        }
        return token;
    }

    void complete(const Token& token) {
        if (!token.active) {
            return;
        }
        const auto t1_ns = now();
        m_values[token.slot] = t1_ns - token.t0_ns;
    }

    std::size_t recorded() const {
        return m_next_slot.load(std::memory_order_relaxed);
    }

    Summary finalize() const {
        if (recorded() != m_expected) {
            throw std::runtime_error("Incomplete latency capture");
        }
        std::vector<std::uint64_t> sorted = m_values;
        std::sort(sorted.begin(), sorted.end());
        Summary summary;
        summary.p50_ns = pick(sorted, 0.50);
        summary.p99_ns = pick(sorted, 0.99);
        summary.p999_ns = pick(sorted, 0.999);
        return summary;
    }

    static std::uint64_t invalid_slot() {
        return std::numeric_limits<std::uint64_t>::max();
    }

    static std::uint64_t now() {
        const auto now_tp = std::chrono::steady_clock::now().time_since_epoch();
        return std::chrono::duration_cast<std::chrono::nanoseconds>(now_tp).count();
    }

private:
    static std::uint64_t pick(const std::vector<std::uint64_t>& data, double percentile) {
        if (data.empty()) {
            return 0;
        }
        const double rank = percentile * static_cast<double>(data.size());
        std::size_t index = static_cast<std::size_t>(rank);
        if (static_cast<double>(index) < rank) {
            index += 1;
        }
        if (index == 0) {
            index = 1;
        }
        const std::size_t pos = std::min<std::size_t>(index - 1, data.size() - 1);
        return data[pos];
    }

    std::vector<std::uint64_t> m_values;
    const std::size_t m_expected;
    std::atomic<std::size_t> m_next_slot;
};

} // namespace logit_bench
