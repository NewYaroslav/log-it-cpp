#include <array>
#include <chrono>
#include <condition_variable>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <sstream>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

#include "LatencyRecorder.hpp"
#include "Scenario.hpp"
#include "adapters/LogItAdapter.hpp"

#ifdef LOGIT_BENCH_HAVE_SPDLOG
#include "adapters/SpdlogAdapter.hpp"
#endif

namespace logit_bench {
namespace {
std::string make_message(std::size_t bytes, std::size_t index) {
    if (bytes == 0) {
        return std::string();
    }
    char fill = static_cast<char>('A' + static_cast<int>(index % 26));
    return std::string(bytes, fill);
}

std::chrono::nanoseconds run_workload(
        ILoggerAdapter& adapter,
        LatencyRecorder& recorder,
        const Scenario& scenario,
        std::size_t total_messages,
        bool record_latency,
        bool measure_duration) {
    std::vector<std::size_t> per_thread(scenario.producers, 0);
    if (scenario.producers == 0) {
        adapter.flush();
        return std::chrono::nanoseconds(0);
    }
    const std::size_t base = total_messages / scenario.producers;
    std::size_t remaining = total_messages % scenario.producers;
    for (std::size_t i = 0; i < scenario.producers; ++i) {
        per_thread[i] = base + (remaining > 0 ? 1 : 0);
        if (remaining > 0) {
            --remaining;
        }
    }

    std::mutex start_mutex;
    std::condition_variable start_cv;
    bool start_flag = false;
    std::size_t ready = 0;

    std::vector<std::thread> threads;
    threads.reserve(scenario.producers);

    for (std::size_t i = 0; i < scenario.producers; ++i) {
        threads.emplace_back([&, i]() {
            std::string message = make_message(scenario.message_bytes, i);
            {
                std::unique_lock<std::mutex> lock(start_mutex);
                ++ready;
                if (ready == scenario.producers) {
                    start_cv.notify_one();
                }
                start_cv.wait(lock, [&]() { return start_flag; });
            }
            for (std::size_t n = 0; n < per_thread[i]; ++n) {
                auto token = recorder.begin(record_latency);
                adapter.log(token, message);
            }
        });
    }

    std::chrono::steady_clock::time_point start_tp;
    if (scenario.producers > 0) {
        std::unique_lock<std::mutex> lock(start_mutex);
        start_cv.wait(lock, [&]() { return ready == scenario.producers; });
        if (measure_duration) {
            start_tp = std::chrono::steady_clock::now();
        }
        start_flag = true;
        start_cv.notify_all();
    }

    for (auto& thread : threads) {
        thread.join();
    }

    adapter.flush();

    if (!measure_duration) {
        return std::chrono::nanoseconds(0);
    }
    auto end_tp = std::chrono::steady_clock::now();
    return std::chrono::duration_cast<std::chrono::nanoseconds>(end_tp - start_tp);
}

struct ScenarioResult {
    LatencyRecorder::Summary summary;
    double throughput = 0.0;
    std::chrono::nanoseconds duration{0};
};

ScenarioResult execute_scenario(
        ILoggerAdapter& adapter,
        const Scenario& scenario,
        std::size_t warmup_messages) {
    LatencyRecorder recorder(scenario.total_messages);
    adapter.prepare(scenario, recorder);
    run_workload(adapter, recorder, scenario, warmup_messages, false, false);
    const auto duration = run_workload(adapter, recorder, scenario, scenario.total_messages, true, true);
    const auto summary = recorder.finalize();
    double throughput = 0.0;
    if (duration.count() > 0) {
        const double seconds = static_cast<double>(duration.count()) / 1'000'000'000.0;
        throughput = static_cast<double>(scenario.total_messages) / seconds;
    }
    return ScenarioResult{summary, throughput, duration};
}

void append_csv(
        const std::string& library,
        const Scenario& scenario,
        const LatencyRecorder::Summary& summary,
        double throughput) {
    namespace fs = std::filesystem;
    const fs::path csv_path{"bench/results/latency.csv"};
    fs::create_directories(csv_path.parent_path());
    bool write_header = false;
    if (!fs::exists(csv_path)) {
        write_header = true;
    } else if (fs::file_size(csv_path) == 0) {
        write_header = true;
    }

    std::ofstream out(csv_path, std::ios::app);
    if (!out) {
        throw std::runtime_error("Failed to open latency.csv for writing");
    }
    if (write_header) {
        out << "lib,async,sink,producers,msg_bytes,total,p50_ns,p99_ns,p999_ns,throughput\n";
    }
    std::ostringstream throughput_stream;
    throughput_stream << std::fixed << std::setprecision(2) << throughput;
    out << library << ','
        << (scenario.async ? 1 : 0) << ','
        << sink_name(scenario.sink) << ','
        << scenario.producers << ','
        << scenario.message_bytes << ','
        << scenario.total_messages << ','
        << summary.p50_ns << ','
        << summary.p99_ns << ','
        << summary.p999_ns << ','
        << throughput_stream.str() << '\n';
}

void print_summary(
        const std::string& library,
        const Scenario& scenario,
        const ScenarioResult& result) {
    std::cout << library
              << " async=" << (scenario.async ? '1' : '0')
              << " sink=" << sink_name(scenario.sink)
              << " producers=" << scenario.producers
              << " bytes=" << scenario.message_bytes
              << " total=" << scenario.total_messages
              << " p50=" << result.summary.p50_ns
              << "ns p99=" << result.summary.p99_ns
              << "ns p999=" << result.summary.p999_ns
              << "ns throughput=" << std::fixed << std::setprecision(2)
              << result.throughput << " msg/s" << std::endl;
}

} // namespace
} // namespace logit_bench

int main() {
    using namespace logit_bench;
    try {
        std::vector<std::unique_ptr<ILoggerAdapter>> adapters;
        adapters.emplace_back(std::make_unique<LogItAdapter>());
#ifdef LOGIT_BENCH_HAVE_SPDLOG
        adapters.emplace_back(std::make_unique<SpdlogAdapter>());
#endif

        const std::array<bool, 2> async_modes{false, true};
        const std::array<SinkKind, 2> sinks{SinkKind::Null, SinkKind::File};
        const std::array<std::size_t, 3> producer_counts{1, 4, 16};
        const std::array<std::size_t, 3> message_sizes{40, 200, 1024};
        constexpr std::size_t total_messages = 6000;
        constexpr std::size_t warmup_messages = 512;

        for (auto& adapter : adapters) {
            for (bool async_mode : async_modes) {
                for (auto sink : sinks) {
                    for (std::size_t producers : producer_counts) {
                        for (std::size_t msg_bytes : message_sizes) {
                            Scenario scenario;
                            scenario.async = async_mode;
                            scenario.sink = sink;
                            scenario.producers = producers;
                            scenario.message_bytes = msg_bytes;
                            scenario.total_messages = total_messages;

                            auto result = execute_scenario(*adapter, scenario, warmup_messages);
                            append_csv(adapter->library_name(), scenario, result.summary, result.throughput);
                            print_summary(adapter->library_name(), scenario, result);
                        }
                    }
                }
            }
        }
    } catch (const std::exception& ex) {
        std::cerr << "Benchmark failed: " << ex.what() << std::endl;
        return 1;
    }
    return 0;
}
