#include <string>
#include <vector>

#include <logit/utils/encoding_utils.hpp>
#include <logit/utils/path_utils.hpp>

namespace odr_fixture {
std::string exec_dir_from_a();
std::vector<std::string> list_some_files(const std::string& base);
std::string file_name_from_a(const std::string& path);
std::string relative_from_a(const std::string& file, const std::string& base);
void ensure_directory_from_a(const std::string& path);
} // namespace odr_fixture

int main() {
    const std::string exec_dir = odr_fixture::exec_dir_from_a();
    odr_fixture::ensure_directory_from_a(exec_dir);

    auto files_from_a = odr_fixture::list_some_files(exec_dir);
    (void)files_from_a;

    const std::string sample_file = exec_dir + "/sample.log";
    auto name_from_a = odr_fixture::file_name_from_a(sample_file);
    (void)name_from_a;

    auto rel_from_a = odr_fixture::relative_from_a(sample_file, exec_dir);
    (void)rel_from_a;

    auto file_direct = logit::get_file_name(exec_dir + "/direct.txt");
    (void)file_direct;

    auto rel_direct = logit::make_relative(exec_dir + "/nested/direct.txt", exec_dir);
    (void)rel_direct;

    logit::create_directories(exec_dir);

    auto list_direct = logit::get_list_files(exec_dir);
    if (list_direct.size() > 5) {
        list_direct.resize(5);
    }
    (void)list_direct;

    return 0;
}
