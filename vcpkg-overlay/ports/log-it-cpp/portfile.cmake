vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO NewYaroslav/log-it-cpp
    REF a5ef55f060f720063bbbb5c9dfdb57086d618e96
    SHA512 1a5a418fcae653ab9731068a21c17a56577f98162d2c292418f60d2722ef3075c83f065c4b6115979dd1f38dd54d50f2ecb635ac0049cafd3cfc2a33aa708fff
    HEAD_REF main
)

vcpkg_cmake_configure(
    SOURCE_PATH "${SOURCE_PATH}"
    OPTIONS -DLOG_IT_CPP_BUILD_TESTS=OFF
)

vcpkg_cmake_install()

vcpkg_cmake_config_fixup(PACKAGE_NAME log-it-cpp CONFIG_PATH lib/cmake/log-it-cpp)

vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/LICENSE")

file(REMOVE_RECURSE
    "${CURRENT_PACKAGES_DIR}/debug/include"
    "${CURRENT_PACKAGES_DIR}/debug/lib"
    "${CURRENT_PACKAGES_DIR}/debug/share/${PORT}"
)
