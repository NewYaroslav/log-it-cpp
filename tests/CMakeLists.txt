if(LOGIT_EMSCRIPTEN)
    add_executable(ems_console ems/console_smoke.cpp)
    target_link_libraries(ems_console PRIVATE log-it-cpp)

    add_executable(ems_async_flush ems/async_flush.cpp)
    target_link_libraries(ems_async_flush PRIVATE log-it-cpp)

    add_executable(ems_single_thread_executor ems/single_thread_executor.cpp)
    target_link_libraries(ems_single_thread_executor PRIVATE log-it-cpp)
else()
    set(TEST_SOURCES
        backpressure_ordering_spsc_test.cpp
        backpressure_ordering_test.cpp
        backpressure_policy_test.cpp
        backend_shutdown_terminal_test.cpp
        compiled_level_runtime_caveat_test.cpp
        compiled_level_test.cpp
        console_logger_dedicated_config_test.cpp
        crash_logger_test.cpp
        dedicated_executor_macro_api_test.cpp
        dedicated_executor_shutdown_test.cpp
        file_logger_current_read_live_test.cpp
        file_logger_external_cmd_compression_test.cpp
        file_logger_file_api_test.cpp
        file_logger_gzip_compression_test.cpp
        file_logger_remove_old_logs_suffixes_test.cpp
        file_logger_rotation_naming_sequence_test.cpp
        file_logger_rotation_naming_timestamp_ms_test.cpp
        file_logger_rotation_naming_timestamp_retention_test.cpp
        file_logger_rotation_naming_timestamp_test.cpp
        file_logger_rotation_test.cpp
        file_logger_test.cpp
        file_logger_set_queue_config_test.cpp
        file_logger_zstd_compression_test.cpp
        fmt_macros_test.cpp
        include_buffered_log_entry_nhr_test.cpp
        include_formatter_nhr_test.cpp
        include_log_file_info_nhr_test.cpp
        include_log_file_read_result_nhr_test.cpp
        include_loggers_nhr_test.cpp
        include_memory_logger_nhr_test.cpp
        include_only_ilogger_test.cpp
        include_quickstart_test.cpp
        include_utils_nhr_test.cpp
        log_filters_tags_test.cpp
        logger_shutdown_race_test.cpp
        logger_snapshot_read_path_test.cpp
        memory_logger_backend_test.cpp
        memory_logger_concurrency_test.cpp
        memory_logger_integration_test.cpp
        os_error_macros_test.cpp
        otlp_http_logger_integration_test.cpp
        otlp_http_logger_callback_test.cpp
        otlp_json_serializer_test.cpp
        otlp_payload_splitter_test.cpp
        otlp_structured_attributes_test.cpp
        otlp_payload_logger_test.cpp
        prometheus_text_serializer_test.cpp
        prometheus_payload_logger_test.cpp
        prometheus_http_server_logger_test.cpp
        prometheus_metric_builders_test.cpp
        per_logger_isolation_test.cpp
        per_logger_mixed_mode_test.cpp
        printf_format_macros_test.cpp
        print_if_macros_test.cpp
        raw_file_logger_test.cpp
        raw_logging_test.cpp
        runtime_log_level_test.cpp
        scope_timer_test.cpp
        single_thread_executor_test.cpp
        task_executor_resize_race_test.cpp
        unique_file_logger_file_api_test.cpp
        unique_file_logger_set_queue_config_test.cpp
        windows_debug_logger_set_queue_config_test.cpp
        windows_debug_macro_compile_test.cpp
    )
    if(NOT LOGIT_WITH_GZIP)
        list(REMOVE_ITEM TEST_SOURCES file_logger_gzip_compression_test.cpp)
        list(REMOVE_ITEM TEST_SOURCES file_logger_external_cmd_compression_test.cpp)
    endif()
    if(NOT LOGIT_WITH_ZSTD)
        list(REMOVE_ITEM TEST_SOURCES file_logger_zstd_compression_test.cpp)
    endif()
    if(NOT LOGIT_WITH_FMT)
        list(REMOVE_ITEM TEST_SOURCES fmt_macros_test.cpp)
    endif()
    if(NOT LOGIT_WITH_OTLP)
        list(REMOVE_ITEM TEST_SOURCES otlp_http_logger_integration_test.cpp)
        list(REMOVE_ITEM TEST_SOURCES otlp_http_logger_callback_test.cpp)
        list(REMOVE_ITEM TEST_SOURCES otlp_structured_attributes_test.cpp)
        list(REMOVE_ITEM TEST_SOURCES otlp_payload_splitter_test.cpp)
        list(REMOVE_ITEM TEST_SOURCES otlp_payload_logger_test.cpp)
    endif()
    if(NOT LOGIT_WITH_PROMETHEUS)
        list(REMOVE_ITEM TEST_SOURCES prometheus_text_serializer_test.cpp)
        list(REMOVE_ITEM TEST_SOURCES prometheus_payload_logger_test.cpp)
        list(REMOVE_ITEM TEST_SOURCES prometheus_http_server_logger_test.cpp)
        list(REMOVE_ITEM TEST_SOURCES prometheus_metric_builders_test.cpp)
    endif()
    if(LOGIT_WITH_PROMETHEUS AND NOT LOGIT_WITH_PROMETHEUS_SERVER)
        list(REMOVE_ITEM TEST_SOURCES prometheus_http_server_logger_test.cpp)
    endif()
    foreach(test_src ${TEST_SOURCES})
        get_filename_component(test_name ${test_src} NAME_WE)
        add_executable(${test_name} ${test_src})
        target_link_libraries(${test_name} PRIVATE log-it-cpp)
        add_test(NAME ${test_name} COMMAND ${test_name})
        if(LOGIT_WITH_OTLP AND test_name MATCHES "^otlp_http_logger_(integration|callback)_test$")
            target_include_directories(${test_name} PRIVATE
                "${CMAKE_CURRENT_SOURCE_DIR}/../external/kurlyk/external/Simple-Web-Server")
        endif()
        if(LOGIT_WITH_PROMETHEUS_SERVER AND test_name STREQUAL "prometheus_http_server_logger_test")
            if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../external/Simple-Web-Server/server_http.hpp")
                target_include_directories(${test_name} PRIVATE
                    "${CMAKE_CURRENT_SOURCE_DIR}/../external/Simple-Web-Server")
            else()
                target_include_directories(${test_name} PRIVATE
                    "${CMAKE_CURRENT_SOURCE_DIR}/../external/kurlyk/external/Simple-Web-Server")
            endif()
            if(WIN32)
                target_link_libraries(${test_name} PRIVATE ws2_32 wsock32)
            endif()
        endif()
        if(test_name STREQUAL "backpressure_policy_test" OR
           test_name STREQUAL "backpressure_ordering_test")
            set_tests_properties(${test_name} PROPERTIES LABELS "tsan")
        endif()
        if(test_name STREQUAL "backpressure_ordering_spsc_test")
            if(MSVC)
                target_compile_options(${test_name} PRIVATE /ULOGIT_USE_MPSC_RING)
            else()
                target_compile_options(${test_name} PRIVATE -ULOGIT_USE_MPSC_RING)
            endif()
        endif()
    endforeach()
endif()
