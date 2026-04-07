#include <logit/utils.hpp>
#include <logit/utils/LogFileReadResult.hpp>

int main() {
    logit::LogFileReadResult result;
    result.file.name = "2026-04-02.log";
    result.content = "hello";
    result.ok = true;
    return result.ok && result.content == "hello" ? 0 : 1;
}
