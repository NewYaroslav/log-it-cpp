cmake_minimum_required(VERSION 3.18)
project(log-it-cpp VERSION 1.0.0 LANGUAGES CXX)

option(LOG_IT_CPP_BUILD_TESTS "Build log-it-cpp tests" ${PROJECT_IS_TOP_LEVEL})
option(LOG_IT_CPP_BUILD_EXAMPLES "Build log-it-cpp examples" OFF)

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

add_compile_definitions(LOGIT_BASE_PATH="$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>")

target_include_directories(log-it-cpp INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/logit_cpp>
    $<INSTALL_INTERFACE:include/logit_cpp>
)

target_link_libraries(log-it-cpp INTERFACE time_shield::time_shield)

if(LOG_IT_CPP_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

if(LOG_IT_CPP_BUILD_EXAMPLES)
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

install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/log-it-cppConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/log-it-cppConfigVersion.cmake"
    DESTINATION lib/cmake/log-it-cpp
)
