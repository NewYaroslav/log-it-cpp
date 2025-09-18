set(LOGIT_BENCH_SOURCES
    main.cpp
    adapters/LogItAdapter.cpp
)

if(LOGIT_BENCH_WITH_SPDLOG)
    list(APPEND LOGIT_BENCH_SOURCES adapters/SpdlogAdapter.cpp)
endif()

add_executable(logit_bench ${LOGIT_BENCH_SOURCES})

target_include_directories(logit_bench PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

target_compile_features(logit_bench PRIVATE cxx_std_17)

target_link_libraries(logit_bench PRIVATE log-it-cpp::log-it-cpp)

if(LOGIT_BENCH_WITH_SPDLOG)
    target_compile_definitions(logit_bench PRIVATE LOGIT_BENCH_HAVE_SPDLOG=1)
    if(NOT TARGET spdlog::spdlog)
        include(FetchContent)
        FetchContent_Declare(spdlog
            GIT_REPOSITORY https://github.com/gabime/spdlog.git
            GIT_TAG v1.12.0
        )
        FetchContent_MakeAvailable(spdlog)
    endif()
    target_link_libraries(logit_bench PRIVATE spdlog::spdlog)
endif()
