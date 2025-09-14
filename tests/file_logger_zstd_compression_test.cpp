#include <LogIt.hpp>
#if defined(LOGIT_HAS_ZSTD)
#include <zstd.h>
#include <string>
#include <vector>
#include <fstream>
#include <cstdlib>

int main() {
    std::system("rm -rf zstd_test");
    logit::FileLogger::Config cfg;
    cfg.directory = "zstd_test";
    cfg.compress = logit::CompressType::ZSTD;
    cfg.compress_level = 3;
    cfg.max_file_size_bytes = 20;
    cfg.compress_async = false;
    logit::Logger::get_instance().add_logger(
        std::unique_ptr<logit::FileLogger>(new logit::FileLogger(cfg)),
        std::unique_ptr<logit::SimpleLogFormatter>(new logit::SimpleLogFormatter("%v")));

    const std::string msg = "0123456789";
    LOGIT_INFO(msg);
    LOGIT_INFO(msg);
    LOGIT_WAIT();
    std::string current = LOGIT_GET_LAST_FILE_PATH(0);
    LOGIT_SHUTDOWN();

    std::string rotated = current;
    size_t pos = rotated.rfind(".log");
    rotated.insert(pos, ".001");
    rotated += ".zst";

    std::ifstream in(rotated.c_str(), std::ios::binary | std::ios::ate);
    if (!in) return 1;
    std::streamsize size = in.tellg();
    in.seekg(0, std::ios::beg);
    std::vector<char> compressed(static_cast<size_t>(size));
    if (!in.read(compressed.data(), size)) return 1;

    unsigned long long raw_size = ZSTD_getFrameContentSize(compressed.data(), compressed.size());
    if (raw_size == ZSTD_CONTENTSIZE_ERROR || raw_size == ZSTD_CONTENTSIZE_UNKNOWN) return 1;
    std::vector<char> decompressed(static_cast<size_t>(raw_size));
    size_t ret = ZSTD_decompress(decompressed.data(), raw_size, compressed.data(), compressed.size());
    if (ZSTD_isError(ret)) return 1;
    std::string out(decompressed.data(), ret);
    return out.find(msg) != std::string::npos ? 0 : 1;
}
#else
int main() { return 0; }
#endif
