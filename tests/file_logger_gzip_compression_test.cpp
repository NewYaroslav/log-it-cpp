#include <LogIt.hpp>
#if defined(LOGIT_HAS_ZLIB)
#include <zlib.h>
#include <string>
#include <cstdlib>

int main() {
    std::system("rm -rf gzip_test");
    logit::FileLogger::Config cfg;
    cfg.directory = "gzip_test";
    cfg.compress = logit::CompressType::GZIP;
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
    rotated += ".gz";

    gzFile gz = gzopen(rotated.c_str(), "rb");
    if (!gz) return 1;
    char buf[128];
    std::string out;
    int n;
    while ((n = gzread(gz, buf, sizeof(buf))) > 0) out.append(buf, n);
    gzclose(gz);
    return out.find(msg) != std::string::npos ? 0 : 1;
}
#else
int main() { return 0; }
#endif
