/// \file example_logit_compression.cpp
/// \brief Demonstrates file log compression using gzip.

#include <logit.hpp>

int main() {
    logit::FileLogger::Config cfg;
    cfg.directory = "gzip_logs";
    cfg.compress = logit::CompressType::GZIP;
    cfg.max_file_size_bytes = 64; // rotate quickly for the demo
    cfg.max_rotated_files = 2;

    LOGIT_ADD_LOGGER(logit::FileLogger, (cfg), logit::SimpleLogFormatter, (LOGIT_CONSOLE_PATTERN));

    for (int i = 0; i < 20; ++i) {
        LOGIT_INFO("compressed log", i);
    }

    LOGIT_WAIT();
    LOGIT_SHUTDOWN();
    return 0;
}
