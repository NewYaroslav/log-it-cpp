#include <logit/utils.hpp>
#include <logit/utils/LogFileInfo.hpp>

int main() {
    logit::LogFileInfo info;
    info.path = "logs/2026-04-02.log";
    info.name = "2026-04-02.log";
    info.day_start_ms = 123456789;
    info.is_current = true;
    info.is_compressed = false;
    return info.is_current && !info.is_compressed ? 0 : 1;
}
