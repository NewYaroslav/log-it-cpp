vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO NewYaroslav/log-it-cpp
    REF 6ae5c3c0c84422695ade31c25191af812474ee93
    SHA512 9045037107c54b3bf9a9f1c9e24968a827dbb77c0fd82c22aa5bfb5ca43ff3b533a51a5ca4f3a435db5df9ebfe58e00c2d5dd9f11aa3f61a0e3d0191ae0eb604
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
