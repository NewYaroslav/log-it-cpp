cmake_minimum_required(VERSION 3.18)
project(log-it-cpp VERSION 1.0.0 LANGUAGES CXX)

option(LOGIT_CPP_BUILD_TESTS "Build log-it-cpp tests" ${PROJECT_IS_TOP_LEVEL})
option(LOGIT_CPP_BUILD_EXAMPLES "Build log-it-cpp examples" OFF)
option(LOGIT_WITH_GZIP "Enable gzip via zlib" OFF)
option(LOGIT_WITH_ZSTD "Enable zstd" OFF)
option(LOGIT_WITH_FMT "Enable fmt support" OFF)
option(LOGIT_USE_SUBMODULES "Allow bundled third_party fallback" OFF)
option(LOGIT_WITH_SYSLOG "Enable POSIX syslog backend" ON)
option(LOGIT_WITH_WIN_EVENT_LOG "Enable Windows Event Log backend" ON)

if(CMAKE_SYSTEM_NAME STREQUAL "Emscripten")
    option(LOGIT_EMSCRIPTEN "Build for Emscripten" ON)
endif()
option(LOGIT_FORCE_ASYNC_OFF "Force disable async logging" OFF)

if(NOT DEFINED CMAKE_CXX_STANDARD)
    set(CMAKE_CXX_STANDARD 11)
endif()
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Dependency: TimeShield
find_package(TimeShield 1.0.3 QUIET CONFIG)
if(NOT TimeShield_FOUND)
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/libs/time-shield-cpp/CMakeLists.txt")
        add_subdirectory(libs/time-shield-cpp)
    else()
        message(FATAL_ERROR "TimeShield not found. Please install it or add as submodule.")
    endif()
endif()

add_library(log-it-cpp INTERFACE)
add_library(log-it-cpp::log-it-cpp ALIAS log-it-cpp)

add_compile_definitions(LOGIT_BASE_PATH="$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>")

target_include_directories(log-it-cpp INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/logit_cpp>
    $<INSTALL_INTERFACE:include/logit_cpp>
)

target_link_libraries(log-it-cpp INTERFACE time_shield::time_shield)

if(LOGIT_EMSCRIPTEN)
    set(LOGIT_WITH_SYSLOG OFF CACHE BOOL "" FORCE)
    set(LOGIT_WITH_WIN_EVENT_LOG OFF CACHE BOOL "" FORCE)
    target_compile_definitions(log-it-cpp INTERFACE LOGIT_EMSCRIPTEN=1)
    target_compile_definitions(log-it-cpp INTERFACE LOGIT_DEFAULT_ASYNC_OFF=1)
    target_link_options(log-it-cpp INTERFACE "-sEXPORTED_RUNTIME_METHODS=['UTF8ToString']")
endif()

if(LOGIT_FORCE_ASYNC_OFF)
    target_compile_definitions(log-it-cpp INTERFACE LOGIT_DEFAULT_ASYNC_OFF=1)
endif()

if(LOGIT_WITH_SYSLOG AND (UNIX OR APPLE) AND NOT EMSCRIPTEN)
    target_compile_definitions(log-it-cpp INTERFACE LOGIT_HAS_SYSLOG=1)
endif()

if(LOGIT_WITH_WIN_EVENT_LOG AND WIN32)
    target_compile_definitions(log-it-cpp INTERFACE LOGIT_HAS_WIN_EVENT_LOG=1)
    target_link_libraries(log-it-cpp INTERFACE advapi32)
endif()

if(LOGIT_WITH_FMT)
    if(NOT TARGET fmt::fmt)
        find_package(fmt QUIET CONFIG)
    endif()
    if(NOT TARGET fmt::fmt AND LOGIT_USE_SUBMODULES)
        add_subdirectory(libs/fmt EXCLUDE_FROM_ALL)
    endif()
    if(NOT TARGET fmt::fmt)
        message(FATAL_ERROR "fmt not found. Enable LOGIT_USE_SUBMODULES or install fmt.")
    endif()
    target_compile_definitions(log-it-cpp INTERFACE LOGIT_WITH_FMT=1)
    target_link_libraries(log-it-cpp INTERFACE fmt::fmt)
endif()

# ---------- GZIP (zlib) ----------
if(LOGIT_WITH_GZIP)
    if(NOT TARGET ZLIB::ZLIB)
        find_package(ZLIB QUIET)
    endif()
    if(NOT TARGET ZLIB::ZLIB AND LOGIT_USE_SUBMODULES)
        add_subdirectory(libs/zlib EXCLUDE_FROM_ALL)
        if(TARGET zlibstatic)
            add_library(ZLIB::ZLIB ALIAS zlibstatic)
        elseif(TARGET zlib)
            add_library(ZLIB::ZLIB ALIAS zlib)
        endif()
    endif()
    if(NOT TARGET ZLIB::ZLIB)
        message(FATAL_ERROR "ZLIB not found. Enable LOGIT_USE_SUBMODULES or install zlib.")
    endif()
    target_compile_definitions(log-it-cpp INTERFACE LOGIT_HAS_ZLIB=1)
    target_link_libraries(log-it-cpp INTERFACE ZLIB::ZLIB)
endif()

# ---------- ZSTD ----------
if(LOGIT_WITH_ZSTD)
    if(NOT TARGET ZSTD::ZSTD)
        find_package(ZSTD QUIET)
    endif()
    if(NOT TARGET ZSTD::ZSTD AND LOGIT_USE_SUBMODULES)
        add_subdirectory(libs/zstd/build/cmake EXCLUDE_FROM_ALL)
        if(TARGET libzstd_static)
            add_library(ZSTD::ZSTD ALIAS libzstd_static)
        elseif(TARGET libzstd_shared)
            add_library(ZSTD::ZSTD ALIAS libzstd_shared)
        endif()
    endif()
    if(NOT TARGET ZSTD::ZSTD)
        message(FATAL_ERROR "ZSTD not found. Enable LOGIT_USE_SUBMODULES or install zstd.")
    endif()
    target_compile_definitions(log-it-cpp INTERFACE LOGIT_HAS_ZSTD=1)
    target_link_libraries(log-it-cpp INTERFACE ZSTD::ZSTD)
endif()

if(DEFINED VCPKG_TARGET_TRIPLET)
    set_property(CACHE LOGIT_USE_SUBMODULES PROPERTY VALUE OFF)
endif()

if(LOGIT_CPP_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

if(LOGIT_CPP_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

include(CMakePackageConfigHelpers)

install(DIRECTORY include/ DESTINATION include)

install(TARGETS log-it-cpp EXPORT log-it-cppTargets)

install(EXPORT log-it-cppTargets
    FILE log-it-cppTargets.cmake
    NAMESPACE log-it-cpp::
    DESTINATION lib/cmake/log-it-cpp
)

write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/log-it-cppConfigVersion.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY AnyNewerVersion
)

  configure_package_config_file(
      cmake/log-it-cppConfig.cmake.in
      "${CMAKE_CURRENT_BINARY_DIR}/log-it-cppConfig.cmake"
      INSTALL_DESTINATION lib/cmake/log-it-cpp
  )

  configure_file(
      cmake/log-it-cpp.pc.in
      "${CMAKE_CURRENT_BINARY_DIR}/log-it-cpp.pc"
      @ONLY
  )

  install(FILES
      "${CMAKE_CURRENT_BINARY_DIR}/log-it-cppConfig.cmake"
      "${CMAKE_CURRENT_BINARY_DIR}/log-it-cppConfigVersion.cmake"
      DESTINATION lib/cmake/log-it-cpp
  )

  install(FILES
      "${CMAKE_CURRENT_BINARY_DIR}/log-it-cpp.pc"
      DESTINATION share/pkgconfig
  )
