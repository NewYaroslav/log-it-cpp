#pragma once

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <vector>
#include <cmath>

namespace logit_bench {

/**
 * Lock-free recorder for latency samples:
 *  - begin(record=true) returns a Token with an assigned slot and t0_ns (steady_clock).
 *  - complete(token) stores (t1-t0) in that slot.
 *  - finalize() returns p50/p99/p99.9 using nearest-rank (ceil) on a sorted copy.
 *
 * Thread-safety: concurrent writers store into distinct preallocated slots.
 */
class LatencyRecorder {
public:
    struct Token {
        std::uint64_t slot = invalid_slot();
        std::uint64_t t0_ns = 0;
        bool active = false;
    };

    struct Summary {
        std::uint64_t p50_ns  = 0;
        std::uint64_t p99_ns  = 0;
        std::uint64_t p999_ns = 0;
    };

    explicit LatencyRecorder(std::size_t total)
        : m_values(total),
          m_expected(total),
          m_next_slot(0) {}

    /**
     * Reserve a slot (if record==true) and capture t0 using steady_clock.
     * We take t0 **after** the slot reservation to minimize skew before log().
     */
    Token begin(bool record) {
        Token token;
        token.active = record;
        if (record) {
            const auto slot = m_next_slot.fetch_add(1, std::memory_order_relaxed);
            if (slot >= m_expected) {
                throw std::out_of_range("LatencyRecorder capacity exceeded");
            }
            token.slot = static_cast<std::uint64_t>(slot);
            token.t0_ns = now();
        }
        return token;
    }

    /// Capture t1 and store (t1 - t0) into the reserved slot.
    void complete(const Token& token) {
        if (!token.active) return;
        const auto t1_ns = now();
        m_values[token.slot] = t1_ns - token.t0_ns; // distinct slots -> no data race
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
        summary.p50_ns  = pick(sorted, 0.50);
        summary.p99_ns  = pick(sorted, 0.99);
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
    // Nearest-rank percentile with ceil(p * N), clamped to [0..N-1].
    static std::uint64_t pick(const std::vector<std::uint64_t>& data, double p) {
        if (data.empty()) return 0;
        const double r = std::ceil(p * static_cast<double>(data.size()));
        std::size_t idx = (r <= 1.0) ? 0 : static_cast<std::size_t>(r) - 1;
        if (idx >= data.size()) idx = data.size() - 1;
        return data[idx];
    }

    std::vector<std::uint64_t> m_values;   // preallocated; no reallocation
    const std::size_t          m_expected; // total messages to record
    std::atomic<std::size_t>   m_next_slot;
};

} // namespace logit_bench
