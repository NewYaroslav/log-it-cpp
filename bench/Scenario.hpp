#pragma once

#include <cstddef>
#include <string>

namespace logit_bench {

enum class SinkKind {
    Null,
    File,
};

inline std::string sink_name(SinkKind sink) {
    switch (sink) {
        case SinkKind::Null: return "null";
        case SinkKind::File: return "file";
    }
    return "unknown";
}

struct Scenario {
    bool        async          = false;
    SinkKind    sink           = SinkKind::Null;
    std::size_t producers      = 1;
    std::size_t message_bytes  = 0;
    std::size_t total_messages = 0;
};

} // namespace logit_bench
