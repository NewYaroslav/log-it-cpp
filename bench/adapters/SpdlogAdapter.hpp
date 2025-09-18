#pragma once

#ifdef LOGIT_BENCH_HAVE_SPDLOG

#include <memory>
#include <string_view>

#include <spdlog/logger.h>

#include "ILoggerAdapter.hpp"

namespace logit_bench {

class SpdlogAdapter : public ILoggerAdapter {
public:
    SpdlogAdapter();
    ~SpdlogAdapter() override;

    const char* library_name() const override { return "spdlog"; }

    void prepare(const Scenario& scenario, LatencyRecorder& recorder) override;

    void log(const LatencyRecorder::Token& token, std::string_view message) override;

    void flush() override;

private:
    class MeasuringSink;

    std::shared_ptr<spdlog::logger> m_logger;
    std::shared_ptr<MeasuringSink> m_sink;
    bool m_async = false;
};

} // namespace logit_bench

#endif // LOGIT_BENCH_HAVE_SPDLOG
