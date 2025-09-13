cmake_minimum_required(VERSION 3.18)
project(install_consumer LANGUAGES CXX)

find_package(log-it-cpp CONFIG REQUIRED)

add_executable(install_consumer main.cpp)
target_link_libraries(install_consumer PRIVATE log-it-cpp::log-it-cpp)
