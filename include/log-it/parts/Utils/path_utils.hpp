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
#endif

namespace logit {

#if __cplusplus >= 201703L

    /// \brief Computes the relative path from base_path to file_path using C++17 std::filesystem.
    /// \param file_path The target file path.
    /// \param base_path The base path from which to compute the relative path.
    /// \return std::string The relative path from base_path to file_path.
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

#else

    /// Structure to hold the root and components of a path.
    struct PathComponents {
        std::string root;                       ///< The root part of the path (e.g., "/", "C:")
        std::vector<std::string> components;    ///< The components of the path.
    };

    /// \brief Splits a path into its root and components.
    /// \param path The path to split.
    /// \return PathComponents The root and components of the path.
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
    /// \return std::string The relative path from base_path to file_path.
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
                    relative_path += '/';
                }
                relative_path += relative_components[i];
            }
        }

        return relative_path;
    }

#endif // __cplusplus >= 201703L

}; // namespace logit

#endif // _LOGIT_PATH_UTILS_HPP_INCLUDED
