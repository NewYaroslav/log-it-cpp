#pragma once

#include <memory>
#include <string_view>

#include "../LatencyRecorder.hpp"
#include "../Scenario.hpp"

namespace logit_bench {

class ILoggerAdapter {
public:
    virtual ~ILoggerAdapter() = default;

    virtual const char* library_name() const = 0;

    virtual void prepare(const Scenario& scenario, LatencyRecorder& recorder) = 0;

    virtual void log(const LatencyRecorder::Token& token, std::string_view message) = 0;

    virtual void flush() = 0;

    virtual void set_recorder_handle(std::shared_ptr<LatencyRecorder>) {}
};

} // namespace logit_bench
