vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO NewYaroslav/log-it-cpp
    REF 9ba0ce1212de3b1f60e22801cca0dc17277e27a3
    SHA512 0094708c32a77aeee4a6de9e99b29f4cd1c7f41bbf69a65c5aad4254bd238e2effb6e669c0b1a035bd242cecf5ef6d048ab5c548dde52450b0b4bc9df371a2b8
    HEAD_REF main
)

vcpkg_cmake_configure(
    SOURCE_PATH "${SOURCE_PATH}"
    MAYBE_UNUSED_VARIABLES LOGIT_WITH_SYSLOG LOGIT_WITH_WIN_EVENT_LOG
    OPTIONS
      -DLOGIT_CPP_BUILD_TESTS=OFF
      -DLOGIT_WITH_SYSLOG=ON
      -DLOGIT_WITH_WIN_EVENT_LOG=ON
)

vcpkg_cmake_install()

vcpkg_cmake_config_fixup(PACKAGE_NAME log-it-cpp CONFIG_PATH lib/cmake/log-it-cpp)

vcpkg_fixup_pkgconfig()

vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/LICENSE")

file(REMOVE_RECURSE
    "${CURRENT_PACKAGES_DIR}/debug"
    "${CURRENT_PACKAGES_DIR}/lib"
)
