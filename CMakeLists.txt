cmake_minimum_required(VERSION 3.18)
project(log-it-cpp VERSION 1.0.2 LANGUAGES CXX)

option(LOGIT_CPP_BUILD_TESTS "Build log-it-cpp tests" ${PROJECT_IS_TOP_LEVEL})
option(LOGIT_CPP_BUILD_EXAMPLES "Build log-it-cpp examples" OFF)
option(LOGIT_BENCH_ENABLE "Build log-it-cpp benchmarks" OFF)
option(LOGIT_BENCH_WITH_SPDLOG "Enable spdlog comparison benchmarks" OFF)
option(LOGIT_WITH_GZIP "Enable gzip via zlib" OFF)
option(LOGIT_WITH_ZSTD "Enable zstd" OFF)
option(LOGIT_WITH_FMT "Enable fmt support" OFF)
option(LOGIT_WITH_CONTEXT "Enable MDC/NDC diagnostic context support" OFF)
option(LOGIT_WITH_OTLP "Enable OTLP/HTTP log export via optional kurlyk dependency" OFF)
option(LOGIT_WITH_PROMETHEUS "Enable Prometheus text payload support" OFF)
option(LOGIT_WITH_PROMETHEUS_SERVER "Enable Prometheus HTTP server backend" OFF)
option(LOGIT_USE_SUBMODULES "Allow bundled optional dependency fallback" OFF)
option(LOGIT_WITH_SYSLOG "Enable POSIX syslog backend" ON)
option(LOGIT_WITH_WIN_EVENT_LOG "Enable Windows Event Log backend" ON)

if(CMAKE_SYSTEM_NAME STREQUAL "Emscripten")
    option(LOGIT_EMSCRIPTEN "Build for Emscripten" ON)
endif()
option(LOGIT_FORCE_ASYNC_OFF "Force disable async logging" OFF)
option(LOGIT_USE_MPSC_RING "Enable lock-free TaskExecutor queue" ON)
option(LOGIT_ENABLE_DROP_OLDEST_SLOWPATH "Enable TaskExecutor DropOldest slow-path" ON)

if(NOT DEFINED CMAKE_CXX_STANDARD)
    if(LOGIT_WITH_OTLP OR LOGIT_WITH_PROMETHEUS_SERVER)
        set(CMAKE_CXX_STANDARD 17)
    else()
        set(CMAKE_CXX_STANDARD 11)
    endif()
endif()
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Dependency: TimeShield
find_package(TimeShield 1.0.4 QUIET CONFIG)
if(NOT TimeShield_FOUND)
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/external/time-shield-cpp/CMakeLists.txt")
        add_subdirectory(external/time-shield-cpp)
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

if(LOGIT_USE_MPSC_RING)
    target_compile_definitions(log-it-cpp INTERFACE LOGIT_USE_MPSC_RING=1)
endif()

if(LOGIT_ENABLE_DROP_OLDEST_SLOWPATH)
    target_compile_definitions(log-it-cpp INTERFACE LOGIT_ENABLE_DROP_OLDEST_SLOWPATH=1)
endif()

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

if(LOGIT_WITH_CONTEXT)
    target_compile_definitions(log-it-cpp INTERFACE LOGIT_WITH_CONTEXT=1)
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
        add_subdirectory(external/fmt EXCLUDE_FROM_ALL)
    endif()
    if(NOT TARGET fmt::fmt)
        message(FATAL_ERROR "fmt not found. Enable LOGIT_USE_SUBMODULES or install fmt.")
    endif()
    target_compile_definitions(log-it-cpp INTERFACE LOGIT_WITH_FMT=1)
    target_link_libraries(log-it-cpp INTERFACE fmt::fmt)
endif()

# ---------- OTLP/HTTP (kurlyk) ----------
if(LOGIT_WITH_OTLP)
    if(EMSCRIPTEN)
        message(FATAL_ERROR "LOGIT_WITH_OTLP is not supported for Emscripten.")
    endif()

    if(NOT TARGET kurlyk)
        find_package(kurlyk QUIET CONFIG)
    endif()

    if(NOT TARGET kurlyk AND LOGIT_USE_SUBMODULES)
        if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/external/kurlyk/CMakeLists.txt")
            set(KURLYK_WEBSOCKET_SUPPORT OFF CACHE BOOL "Disable kurlyk WebSocket support for LogIt++ OTLP" FORCE)
            set(KURLYK_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)

            if(WIN32 AND MINGW)
                set(KURLYK_USE_FALLBACK_OPENSSL ON CACHE BOOL "Use bundled OpenSSL fallback for MinGW" FORCE)
                set(KURLYK_USE_FALLBACK_CURL ON CACHE BOOL "Use bundled curl fallback for MinGW" FORCE)
                set(KURLYK_USE_FALLBACK_ASIO ON CACHE BOOL "Use bundled Asio fallback for MinGW" FORCE)
                set(KURLYK_USE_FALLBACK_SIMPLE_WS_SERVER ON CACHE BOOL "Use bundled Simple-WebSocket-Server fallback for MinGW" FORCE)
            endif()

            add_subdirectory(external/kurlyk EXCLUDE_FROM_ALL)
        endif()
    endif()

    if(NOT TARGET kurlyk)
        message(FATAL_ERROR "kurlyk not found. Install kurlyk or add it as external/kurlyk and enable LOGIT_USE_SUBMODULES.")
    endif()

    target_compile_definitions(log-it-cpp INTERFACE LOGIT_WITH_OTLP=1)
    target_compile_definitions(log-it-cpp INTERFACE KURLYK_WEBSOCKET_SUPPORT=0)
    target_compile_definitions(log-it-cpp INTERFACE KURLYK_HTTP_SUPPORT=1)
    target_link_libraries(log-it-cpp INTERFACE kurlyk)
endif()

# ---------- Prometheus ----------
if(LOGIT_WITH_PROMETHEUS)
    if(EMSCRIPTEN)
        message(FATAL_ERROR "LOGIT_WITH_PROMETHEUS is not supported for Emscripten.")
    endif()
    target_compile_definitions(log-it-cpp INTERFACE LOGIT_WITH_PROMETHEUS=1)
endif()

if(LOGIT_WITH_PROMETHEUS_SERVER)
    if(EMSCRIPTEN)
        message(FATAL_ERROR "LOGIT_WITH_PROMETHEUS_SERVER is not supported for Emscripten.")
    endif()

    target_compile_definitions(log-it-cpp INTERFACE ASIO_STANDALONE)

    # Prefer standalone external/Simple-Web-Server
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/external/Simple-Web-Server/server_http.hpp")
        target_include_directories(log-it-cpp INTERFACE
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/external/Simple-Web-Server>
        )
        if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/external/Simple-Web-Server/../asio/include/asio.hpp")
            target_include_directories(log-it-cpp INTERFACE
                $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/external/asio/include>
            )
        endif()
    elseif(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/external/kurlyk/external/Simple-Web-Server/server_http.hpp")
        target_include_directories(log-it-cpp INTERFACE
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/external/kurlyk/external/Simple-Web-Server>
        )
        if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/external/kurlyk/external/asio/include/asio.hpp")
            target_include_directories(log-it-cpp INTERFACE
                $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/external/kurlyk/external/asio/include>
            )
        endif()
    else()
        message(FATAL_ERROR "Simple-Web-Server not found. Add it as external/Simple-Web-Server or enable/provide kurlyk submodule.")
    endif()

    if(NOT LOGIT_WITH_PROMETHEUS)
        set(LOGIT_WITH_PROMETHEUS ON)
        target_compile_definitions(log-it-cpp INTERFACE LOGIT_WITH_PROMETHEUS=1)
    endif()
    target_compile_definitions(log-it-cpp INTERFACE LOGIT_WITH_PROMETHEUS_SERVER=1)
    if(WIN32)
        target_link_libraries(log-it-cpp INTERFACE ws2_32 wsock32)
    endif()
endif()

# ---------- GZIP (zlib) ----------
if(LOGIT_WITH_GZIP)
    if(NOT TARGET ZLIB::ZLIB)
        find_package(ZLIB QUIET)
    endif()
    if(NOT TARGET ZLIB::ZLIB AND LOGIT_USE_SUBMODULES)
        add_subdirectory(external/zlib EXCLUDE_FROM_ALL)
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
        add_subdirectory(external/zstd/build/cmake EXCLUDE_FROM_ALL)
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

if(LOGIT_BENCH_ENABLE)
    add_subdirectory(bench)
endif()

include(CMakePackageConfigHelpers)

install(DIRECTORY include/ DESTINATION include)

install(TARGETS log-it-cpp EXPORT log-it-cppTargets)

# install(EXPORT) requires all linked targets to be in an export set.
# When kurlyk is pre-installed (IMPORTED target) the export works.
# When kurlyk is a submodule (non-IMPORTED), packaging is unsupported.
# The FATAL_ERROR is deferred to install time so development builds with
# submodules still work; only `cmake --install` is blocked.
if(LOGIT_WITH_OTLP AND TARGET kurlyk)
    get_target_property(_kurlyk_imported kurlyk IMPORTED)
    if(NOT _kurlyk_imported)
        install(CODE [[
            message(FATAL_ERROR
                "log-it-cpp: Installing with LOGIT_WITH_OTLP=ON and bundled kurlyk is not supported. "
                "Install kurlyk separately and use find_package(kurlyk), or disable LOGIT_WITH_OTLP for install.")
        ]])
    else()
        install(EXPORT log-it-cppTargets
            FILE log-it-cppTargets.cmake
            NAMESPACE log-it-cpp::
            DESTINATION lib/cmake/log-it-cpp
        )
    endif()
else()
    install(EXPORT log-it-cppTargets
        FILE log-it-cppTargets.cmake
        NAMESPACE log-it-cpp::
        DESTINATION lib/cmake/log-it-cpp
    )
endif()

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
