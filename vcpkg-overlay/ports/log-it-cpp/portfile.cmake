vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO NewYaroslav/log-it-cpp
    REF b1bceedddf34b417eb8b81de3508fe9c9704a46a
    SHA512 f3781cbb4e9e190df38c3fe7fa80ba69bf6f9dbafb158e0426dd4604f2f1ba794450679005a38d0f9f1dad0696e2f22b8b086b2d7d08a0f99bb4fd3b0f7ed5d8
    HEAD_REF main
)

vcpkg_cmake_configure(
    SOURCE_PATH "${SOURCE_PATH}"
    OPTIONS
        -DLOG_IT_CPP_BUILD_TESTS=OFF
)

vcpkg_cmake_install()

vcpkg_cmake_config_fixup(
    PACKAGE_NAME log-it-cpp
    CONFIG_PATH lib/cmake/log-it-cpp
)

vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/LICENSE")

file(REMOVE_RECURSE
    "${CURRENT_PACKAGES_DIR}/debug/include"
    "${CURRENT_PACKAGES_DIR}/debug/lib"
    "${CURRENT_PACKAGES_DIR}/debug/share/${PORT}"
)
