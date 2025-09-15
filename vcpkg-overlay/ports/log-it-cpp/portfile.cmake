vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO NewYaroslav/log-it-cpp
    REF 7fab3b2acba6fe78a136e0694ec7d999a0ba3ba4
    SHA512 100b515b31ffce25e5b24f8fd23767b20d015d9e0100024c330f3638a5db85dfb1e63821c09ea214bd744841e0a8d18542e60a49fec8528a01402ba9a5e21830
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
