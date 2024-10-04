#ifndef _LOGIT_PATH_UTILS_HPP_INCLUDED
#define _LOGIT_PATH_UTILS_HPP_INCLUDED
/// \file path_utils.hpp
/// \brief Utility functions for path manipulation, including relative path computation.

#include <string>
#if __cplusplus >= 201703L
#include <filesystem>
#else
#include <vector>
#include <cctype>
#include <stdexcept>
#endif

#ifdef _WIN32
// For Windows systems
#include <direct.h>
#include <windows.h>
#include <locale>
#include <codecvt>
#else
// For POSIX systems
#include <unistd.h>
#include <limits.h>
#include <dirent.h>
#include <sys/stat.h>
#include <errno.h>
#endif

namespace logit {
#   if __cplusplus >= 201703L
    namespace fs = std::filesystem;
#   endif

    /// \brief Retrieves the directory of the executable file.
    /// \return A string containing the directory path of the executable.
    std::string get_exe_path() {
#       ifdef _WIN32
        std::vector<wchar_t> buffer(MAX_PATH);
        HMODULE hModule = GetModuleHandle(NULL);

        // Пробуем получить путь
        DWORD size = GetModuleFileNameW(hModule, buffer.data(), buffer.size());

        // Если путь слишком длинный, увеличиваем буфер
        while (size == buffer.size() && GetLastError() == ERROR_INSUFFICIENT_BUFFER) {
            buffer.resize(buffer.size() * 2);  // Увеличиваем буфер в два раза
            size = GetModuleFileNameW(hModule, buffer.data(), buffer.size());
        }

        if (size == 0) {
            throw std::runtime_error("Failed to get executable path.");
        }

        std::wstring exe_path(buffer.begin(), buffer.begin() + size);

        // Обрезаем путь до директории (удаляем имя файла, оставляем только путь к папке)
        size_t pos = exe_path.find_last_of(L"\\/");
        if (pos != std::wstring::npos) {
            exe_path = exe_path.substr(0, pos);
        }

        // Преобразуем из std::wstring (UTF-16) в std::string (UTF-8)
        std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
        return converter.to_bytes(exe_path);
#       else
        char result[PATH_MAX];
        ssize_t count = readlink("/proc/self/exe", result, PATH_MAX);

        if (count == -1) {
            throw std::runtime_error("Failed to get executable path.");
        }

        std::string exe_path(result, count);

        // Обрезаем путь до директории (удаляем имя файла, оставляем только путь к папке)
        size_t pos = exe_path.find_last_of("\\/");
        if (pos != std::string::npos) {
            exe_path = exe_path.substr(0, pos);
        }

        return exe_path;
#   endif
    }

    /// \brief Recursively retrieves a list of all files in a directory.
    /// \param path The directory path to search.
    /// \return A vector of strings containing the full paths of all files found.
    std::vector<std::string> get_list_files(const std::string& path) {
        std::vector<std::string> list_files;
        std::string search_path = path;

        // Handle empty path by using current working directory
        if (search_path.empty()) {
#           ifdef _WIN32
            char buffer[MAX_PATH];
            GetCurrentDirectoryA(MAX_PATH, buffer);
            search_path = buffer;
#           else
            char buffer[PATH_MAX];
            if (getcwd(buffer, PATH_MAX)) {
                search_path = buffer;
            }
#           endif
        }

        // Ensure the path ends with the appropriate separator
#       ifdef _WIN32
        char path_separator = '\\';
#       else
        char path_separator = '/';
#       endif

        if (search_path.back() != '/' && search_path.back() != '\\') {
            search_path += path_separator;
        }

#       ifdef _WIN32
        std::string pattern = search_path + "*";
        WIN32_FIND_DATAA fd;
        HANDLE hFind = FindFirstFileA(pattern.c_str(), &fd);

        if (hFind != INVALID_HANDLE_VALUE) {
            do {
                std::string file_name = fd.cFileName;

                if (file_name == "." || file_name == "..") {
                    continue;
                }

                std::string full_path = search_path + file_name;

                if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                    // Recurse into subdirectories
                    std::vector<std::string> sub_files = get_list_files(full_path);
                    list_files.insert(list_files.end(), sub_files.begin(), sub_files.end());
                } else {
                    // Add files to the list
                    list_files.push_back(full_path);
                }
            } while (FindNextFileA(hFind, &fd));
            FindClose(hFind);
        }
#       else
        DIR* dir = opendir(search_path.c_str());
        if (dir) {
            struct dirent* entry;
            while ((entry = readdir(dir)) != nullptr) {
                std::string file_name = entry->d_name;

                if (file_name == "." || file_name == "..") {
                    continue;
                }

                std::string full_path = search_path + file_name;
                struct stat statbuf;

                if (stat(full_path.c_str(), &statbuf) == 0) {
                    if (S_ISDIR(statbuf.st_mode)) {
                        // Recurse into subdirectories
                        std::vector<std::string> sub_files = get_list_files(full_path);
                        list_files.insert(list_files.end(), sub_files.begin(), sub_files.end());
                    } else if (S_ISREG(statbuf.st_mode)) {
                        // Add files to the list
                        list_files.push_back(full_path);
                    }
                }
            }
            closedir(dir);
        }
#       endif
        return list_files;
    }

#if __cplusplus >= 201703L

    /// \brief Computes the relative path from base_path to file_path using C++17 std::filesystem.
    /// \param file_path The target file path.
    /// \param base_path The base path from which to compute the relative path.
    /// \return A string representing the relative path from base_path to file_path.
    inline std::string make_relative(const std::string& file_path, const std::string& base_path) {
        if (base_path.empty()) return file_path;
        std::filesystem::path fileP = std::filesystem::u8path(file_path);
        std::filesystem::path baseP = std::filesystem::u8path(base_path);
        std::error_code ec; // For exception-safe operation
        std::filesystem::path relativeP = std::filesystem::relative(fileP, baseP, ec);
        if (ec) {
            // If there is an error, return the original file_path
            return file_path;
        } else {
            return relativeP.u8string();
        }
    }

    /// \brief Creates directories recursively for the given path using C++17 std::filesystem.
    /// \param path The directory path to create.
    /// \throws std::runtime_error if the directories cannot be created.
    void create_directories(const std::string& path) {
        std::filesystem::path dir(path);
        if (!std::filesystem::exists(dir)) {
            std::error_code ec;
            if (!std::filesystem::create_directories(dir, ec)) {
                throw std::runtime_error("Failed to create directories for path: " + path);
            }
        }
    }

#else

    /// \struct PathComponents
    /// \brief Structure to hold the root and components of a path.
    struct PathComponents {
        std::string root;                       ///< The root part of the path (e.g., "/", "C:")
        std::vector<std::string> components;    ///< The components of the path.
    };

    /// \brief Splits a path into its root and components.
    /// \param path The path to split.
    /// \return A PathComponents object containing the root and components of the path.
    PathComponents split_path(const std::string& path) {
        PathComponents result;
        size_t i = 0;
        size_t n = path.size();

        // Handle root paths for Unix and Windows
        if (n >= 1 && (path[0] == '/' || path[0] == '\\')) {
            // Unix root "/"
            result.root = "/";
            ++i;
        } else if (n >= 2 && std::isalpha(path[0]) && path[1] == ':') {
            // Windows drive letter "C:"
            result.root = path.substr(0, 2);
            i = 2;
            if (n >= 3 && (path[2] == '/' || path[2] == '\\')) {
                // "C:/"
                ++i;
            }
        }

        // Split the path into components
        while (i < n) {
            // Skip path separators
            while (i < n && (path[i] == '/' || path[i] == '\\')) {
                ++i;
            }
            // Find the next separator
            size_t j = i;
            while (j < n && path[j] != '/' && path[j] != '\\') {
                ++j;
            }
            if (i < j) {
                result.components.push_back(path.substr(i, j - i));
                i = j;
            }
        }

        return result;
    }

    /// \brief Computes the relative path from base_path to file_path.
    /// \param file_path The target file path.
    /// \param base_path The base path from which to compute the relative path.
    /// \return A string representing the relative path from base_path to file_path.
    std::string make_relative(const std::string& file_path, const std::string& base_path) {
        if (base_path.empty()) return file_path;
        PathComponents file_pc = split_path(file_path);
        PathComponents base_pc = split_path(base_path);

        // If roots are different, return the original file_path
        if (file_pc.root != base_pc.root) {
            return file_path;
        }

        // Find the common prefix components
        size_t common_size = 0;
        while (common_size < file_pc.components.size() &&
               common_size < base_pc.components.size() &&
               file_pc.components[common_size] == base_pc.components[common_size]) {
            ++common_size;
        }

        // Build the relative path components
        std::vector<std::string> relative_components;

        // Add ".." for each remaining component in base path
        for (size_t i = common_size; i < base_pc.components.size(); ++i) {
            relative_components.push_back("..");
        }

        // Add the remaining components from the file path
        for (size_t i = common_size; i < file_pc.components.size(); ++i) {
            relative_components.push_back(file_pc.components[i]);
        }

        // Join the components into a relative path string
        std::string relative_path;
        if (relative_components.empty()) {
            relative_path = ".";
        } else {
            for (size_t i = 0; i < relative_components.size(); ++i) {
                if (i > 0) {
#                   ifdef _WIN32
                    relative_path += '\\';  // Windows
#                   else
                    relative_path += '/';
#                   endif
                }
                relative_path += relative_components[i];
            }
        }

        return relative_path;
    }

    /// \brief Checks if a path represents a file (by checking for an extension).
    /// \param path The path to check.
    /// \return True if the path represents a file, false otherwise.
    inline bool is_file(const std::string& path) {
        size_t dot_pos = path.find_last_of('.');
        size_t slash_pos = path.find_last_of("/\\");
        return (dot_pos != std::string::npos && (slash_pos == std::string::npos || dot_pos > slash_pos));
    }

    /// \brief Creates directories recursively for the given path.
    /// \param path The directory path to create.
    /// \throws std::runtime_error if the directories cannot be created.
    void create_directories(const std::string& path) {
        if (path.empty()) return;
        PathComponents path_pc = split_path(path);
        auto &components = path_pc.components;
        size_t components_size = components.size();

        // Check if the last component is a file
        if (is_file(path)) {
            --components_size;
        }

        // Build the path incrementally and create directories
        std::string current_path = path_pc.root;
        for (size_t i = 0; i < components_size; ++i) {
            if (!current_path.empty() && current_path.back() != '/' && current_path.back() != '\\') {
                current_path += '/';
            }
            current_path += components[i];

            /*
            if (i) current_path += components[i] + "/";
            else if (i == 0 &&
                    (components[i] == "/" ||
                     components[i] == "~/")) {
                current_path += components[i];
            } else {
                current_path += components[i] + "/";
            }
            */

            // Skip special components
            if (components[i] == ".." ||
                components[i] == "/" ||
                components[i] == "~/") continue;
#           ifdef _WIN32
            int ret = _mkdir(current_path.c_str());
#           else
            int ret = mkdir(current_path.c_str(), 0755);
#           endif
            int errnum = errno;
            if (ret != 0 && errnum != EEXIST) {
                throw std::runtime_error("Failed to create directory: " + current_path);
            }
        }
    }

#endif // __cplusplus >= 201703L

}; // namespace logit

#endif // _LOGIT_PATH_UTILS_HPP_INCLUDED
