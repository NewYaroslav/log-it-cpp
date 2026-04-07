#include <logit.hpp>
#include <fstream>
#include <string>
#include <vector>
#include <thread>
#include <chrono>
#if __cplusplus >= 201703L
#include <filesystem>
#endif

namespace {

std::string make_unique_directory_name(const std::string& prefix) {
    const long long stamp = static_cast<long long>(
        std::chrono::steady_clock::now().time_since_epoch().count());
    return prefix + "_" + std::to_string(stamp);
}

bool contains_text(const std::string& haystack, const std::string& needle) {
    return haystack.find(needle) != std::string::npos;
}

bool same_file_name(const std::string& lhs, const std::string& rhs) {
    return logit::get_file_name(lhs) == logit::get_file_name(rhs);
}

size_t find_index_by_name(
        const std::vector<logit::LogFileInfo>& files,
        const std::string& path) {
    for (size_t i = 0; i < files.size(); ++i) {
        if (same_file_name(files[i].path, path)) {
            return i;
        }
    }
    return files.size();
}

} // namespace

int main() {
    const std::string directory_name = make_unique_directory_name("unique_file_api_logs");

    logit::UniqueFileLogger::Config cfg;
    cfg.directory = directory_name;
    cfg.async = false;

    logit::Logger::get_instance().add_logger(
        std::unique_ptr<logit::UniqueFileLogger>(new logit::UniqueFileLogger(cfg)),
        std::unique_ptr<logit::SimpleLogFormatter>(new logit::SimpleLogFormatter("%v")));
    LOGIT_ADD_MEMORY_LOGGER_DEFAULT_SINGLE_MODE();

    const std::string first = "unique-first";
    const std::string second = "unique-second";
    LOGIT_INFO(first);
    const std::string first_path = LOGIT_GET_LAST_FILE_PATH(0);
    std::this_thread::sleep_for(std::chrono::milliseconds(2));
    LOGIT_INFO(second);

    const std::string latest_path = LOGIT_GET_LAST_FILE_PATH(0);
    if (first_path.empty() || latest_path.empty()) {
        return 1;
    }

    std::string compressed_path;
    {
        const std::vector<logit::LogFileInfo> plain_files = LOGIT_LIST_LOG_FILES(0);
        if (plain_files.size() < 2) {
            return 1;
        }
        compressed_path = plain_files[0].path + ".gz";
        std::ofstream gz(compressed_path.c_str(), std::ios_base::binary);
        gz << "compressed-placeholder";
    }

    const std::vector<logit::LogFileInfo> files = LOGIT_LIST_LOG_FILES(0);
    if (files.size() < 3) {
        return 1;
    }

    bool found_compressed = false;
    for (size_t i = 0; i < files.size(); ++i) {
        if (files[i].is_current) {
            return 1;
        }
        if (same_file_name(files[i].path, compressed_path) && files[i].is_compressed) {
            found_compressed = true;
        }
    }
    if (!found_compressed) {
        return 1;
    }

    const size_t latest_index = find_index_by_name(files, latest_path);
    const size_t first_index = find_index_by_name(files, first_path);
    const size_t compressed_index = find_index_by_name(files, compressed_path);
    if (latest_index == files.size() ||
        first_index == files.size() ||
        compressed_index == files.size()) {
        return 1;
    }
    if (!(latest_index < first_index && latest_index < compressed_index)) {
        return 1;
    }

    const logit::LogFileReadResult latest_read = LOGIT_READ_LOG_FILE(0, latest_path);
    if (!latest_read.ok || !contains_text(latest_read.content, second)) {
        return 1;
    }

    const logit::LogFileReadResult compressed_read = LOGIT_READ_LOG_FILE(0, compressed_path);
    if (compressed_read.ok || !compressed_read.content.empty()) {
        return 1;
    }

    const std::vector<std::string> requested_paths = {latest_path, compressed_path};
    const std::vector<logit::LogFileReadResult> read_many =
        LOGIT_READ_LOG_FILES(0, requested_paths);
    if (read_many.size() != requested_paths.size()) {
        return 1;
    }
    if (!same_file_name(read_many[0].file.path, latest_path) || !read_many[0].ok ||
        !same_file_name(read_many[1].file.path, compressed_path) || read_many[1].ok) {
        return 1;
    }

    if (!LOGIT_LIST_LOG_FILES(1).empty()) {
        return 1;
    }
    if (LOGIT_READ_LOG_FILE(1, latest_path).ok) {
        return 1;
    }

    LOGIT_SHUTDOWN();
    return 0;
}
