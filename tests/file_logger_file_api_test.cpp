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

std::string make_rotated_path(const std::string& current_path) {
    std::string rotated = current_path;
    const size_t pos = rotated.rfind(".log");
    if (pos != std::string::npos) {
        rotated.insert(pos, ".001");
    }
    return rotated;
}

bool contains_text(const std::string& haystack, const std::string& needle) {
    return haystack.find(needle) != std::string::npos;
}

bool same_file_name(const std::string& lhs, const std::string& rhs) {
    return logit::get_file_name(lhs) == logit::get_file_name(rhs);
}

std::string get_directory(const std::string& path) {
    const size_t pos = path.find_last_of("/\\");
    return pos == std::string::npos ? std::string() : path.substr(0, pos);
}

std::string get_date_base(const std::string& path) {
    const std::string filename = logit::get_file_name(path);
    const size_t pos = filename.rfind(".log");
    return pos == std::string::npos ? filename : filename.substr(0, pos);
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
    const std::string directory_name = make_unique_directory_name("file_api_logs");

    logit::FileLogger::Config cfg;
    cfg.directory = directory_name;
    cfg.async = false;
    cfg.max_file_size_bytes = 20;

    logit::Logger::get_instance().add_logger(
        std::unique_ptr<logit::FileLogger>(new logit::FileLogger(cfg)),
        std::unique_ptr<logit::SimpleLogFormatter>(new logit::SimpleLogFormatter("%v")));
    LOGIT_ADD_MEMORY_LOGGER_DEFAULT_SINGLE_MODE();

    const std::string first = "0123456789";
    const std::string second = "abcdefghij";

    LOGIT_INFO(first);
    LOGIT_INFO(second);

    const std::string current_path = LOGIT_GET_LAST_FILE_PATH(0);
    const std::string rotated_path = make_rotated_path(current_path);
    const std::string compressed_path = rotated_path + ".gz";
    const std::string directory = get_directory(current_path);
    const std::string date_base = get_date_base(current_path);
    const std::string timestamp_9_path = directory + "/" + date_base + "_120000.9.log";
    const std::string timestamp_10_path = directory + "/" + date_base + "_120000.10.log";
    const std::string timestamp_10_compressed_path = timestamp_10_path + ".gz";

    {
        std::ofstream gz(compressed_path.c_str(), std::ios_base::binary);
        gz << "compressed-placeholder";
    }
    {
        std::ofstream rotated_ts9(timestamp_9_path.c_str(), std::ios_base::binary);
        rotated_ts9 << "timestamp-nine";
    }
    {
        std::ofstream rotated_ts10(timestamp_10_path.c_str(), std::ios_base::binary);
        rotated_ts10 << "timestamp-ten";
    }
    {
        std::ofstream rotated_ts10_gz(timestamp_10_compressed_path.c_str(), std::ios_base::binary);
        rotated_ts10_gz << "timestamp-ten-compressed";
    }

    const std::vector<logit::LogFileInfo> files = LOGIT_LIST_LOG_FILES(0);
    if (files.size() < 6) {
        return 1;
    }

    bool found_current = false;
    bool found_rotated = false;
    bool found_compressed = false;
    for (size_t i = 0; i < files.size(); ++i) {
        if (same_file_name(files[i].path, current_path) &&
            files[i].is_current && !files[i].is_compressed) {
            found_current = true;
        }
        if (same_file_name(files[i].path, rotated_path) &&
            !files[i].is_current && !files[i].is_compressed) {
            found_rotated = true;
        }
        if (same_file_name(files[i].path, compressed_path) && files[i].is_compressed) {
            found_compressed = true;
        }
    }
    if (!found_current || !found_rotated || !found_compressed) {
        return 1;
    }

    const size_t current_index = find_index_by_name(files, current_path);
    const size_t rotated_index = find_index_by_name(files, rotated_path);
    const size_t compressed_index = find_index_by_name(files, compressed_path);
    const size_t timestamp_9_index = find_index_by_name(files, timestamp_9_path);
    const size_t timestamp_10_index = find_index_by_name(files, timestamp_10_path);
    const size_t timestamp_10_compressed_index =
        find_index_by_name(files, timestamp_10_compressed_path);
    if (current_index == files.size() ||
        rotated_index == files.size() ||
        compressed_index == files.size() ||
        timestamp_9_index == files.size() ||
        timestamp_10_index == files.size() ||
        timestamp_10_compressed_index == files.size()) {
        return 1;
    }
    if (!(current_index < rotated_index &&
          rotated_index < compressed_index &&
          current_index < timestamp_10_index &&
          timestamp_10_index < timestamp_9_index &&
          timestamp_10_index < timestamp_10_compressed_index)) {
        return 1;
    }

    const logit::LogFileReadResult current_read = LOGIT_READ_LOG_FILE(0, current_path);
    if (!current_read.ok || !contains_text(current_read.content, second)) {
        return 1;
    }

    const logit::LogFileReadResult rotated_read = LOGIT_READ_LOG_FILE(0, rotated_path);
    if (!rotated_read.ok || !contains_text(rotated_read.content, first)) {
        return 1;
    }

    const logit::LogFileReadResult compressed_read = LOGIT_READ_LOG_FILE(0, compressed_path);
    if (compressed_read.ok || !compressed_read.content.empty()) {
        return 1;
    }

    const std::vector<std::string> requested_paths =
        {rotated_path, current_path, compressed_path};
    const std::vector<logit::LogFileReadResult> read_many =
        LOGIT_READ_LOG_FILES(0, requested_paths);
    if (read_many.size() != requested_paths.size()) {
        return 1;
    }
    if (!same_file_name(read_many[0].file.path, rotated_path) || !read_many[0].ok ||
        !same_file_name(read_many[1].file.path, current_path) || !read_many[1].ok ||
        !same_file_name(read_many[2].file.path, compressed_path) || read_many[2].ok) {
        return 1;
    }

    if (!LOGIT_LIST_LOG_FILES(1).empty()) {
        return 1;
    }
    if (LOGIT_READ_LOG_FILE(1, current_path).ok) {
        return 1;
    }

    if (!LOGIT_LIST_LOG_FILES(99).empty()) {
        return 1;
    }
    if (LOGIT_READ_LOG_FILE(99, current_path).ok) {
        return 1;
    }

    LOGIT_SHUTDOWN();
    return 0;
}
