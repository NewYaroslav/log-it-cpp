#include <string>
#include <vector>

#include <logit/utils/encoding_utils.hpp>
#include <logit/utils/path_utils.hpp>

namespace odr_fixture {

std::string exec_dir_from_a() {
    return logit::get_exec_dir();
}

std::vector<std::string> list_some_files(const std::string& base) {
    auto files = logit::get_list_files(base);
    if (files.size() > 4) {
        files.resize(4);
    }
    return files;
}

std::string file_name_from_a(const std::string& path) {
    return logit::get_file_name(path);
}

std::string relative_from_a(const std::string& file, const std::string& base) {
    return logit::make_relative(file, base);
}

void ensure_directory_from_a(const std::string& path) {
    logit::create_directories(path);
}

} // namespace odr_fixture
