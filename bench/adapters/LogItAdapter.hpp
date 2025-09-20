#pragma once

#include <memory>
#include <string>
#include <logit.hpp>

#include "ILoggerAdapter.hpp"

namespace logit_bench {

class LogItAdapter : public ILoggerAdapter {
public:
    LogItAdapter();
    ~LogItAdapter() override;

    const char* library_name() const override { return "log-it-cpp"; }

    void prepare(const Scenario& scenario, LatencyRecorder& recorder) override;

    void log(const LatencyRecorder::Token& token, std::string_view message) override;

    void flush() override;

private:
    class Impl;
    std::unique_ptr<Impl> m_impl;
};

} // namespace logit_bench
