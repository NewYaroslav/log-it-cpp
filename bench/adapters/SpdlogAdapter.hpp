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

    void set_recorder_handle(std::shared_ptr<LatencyRecorder> recorder) override;

private:
    class MeasuringSink;

    std::shared_ptr<spdlog::logger> m_logger;
    std::shared_ptr<MeasuringSink> m_sink;
    std::shared_ptr<LatencyRecorder> m_recorder_handle;
    bool m_async = false;
};

} // namespace logit_bench

#endif // LOGIT_BENCH_HAVE_SPDLOG
