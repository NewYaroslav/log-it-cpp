# При генерации Code::Blocks проекта с MinGW можно использовать так:
# cmake -G "CodeBlocks - MinGW Makefiles" -S . -B build-cb
cmake_minimum_required(VERSION 3.18)
project(time_shield LANGUAGES CXX)

if(MINGW OR CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    set(COMMON_WARN_FLAGS -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion -Wshadow)
endif()

if(MSVC)
    set(COMMON_WARN_FLAGS /W4 /wd4996)
endif()

if(NOT DEFINED CMAKE_CXX_STANDARD)
    set(CMAKE_CXX_STANDARD 11)
endif()
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Абсолютный путь к корню проекта (где CMakeLists.txt)
get_filename_component(PROJECT_ROOT "${CMAKE_CURRENT_SOURCE_DIR}" ABSOLUTE)

# Добавляем макрос LOGIT_BASE_PATH
add_compile_definitions(LOGIT_BASE_PATH="${PROJECT_ROOT}")

# Пути к инклудам и библиотекам
set(PROJECT_INCLUDE_DIRS
    ${CMAKE_CURRENT_SOURCE_DIR}/include/logit_cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/libs/fmt/include
    ${CMAKE_CURRENT_SOURCE_DIR}/libs/time-shield-cpp/include/time_shield_cpp
)

# Заголовочные файлы из include/
file(GLOB_RECURSE PROJECT_HEADERS
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    include/*.hpp
)

# Создаём виртуальную цель для заголовков, чтобы отображались в IDE
add_custom_target(project_headers SOURCES ${PROJECT_HEADERS})

# Находим все .cpp файлы в examples/
file(GLOB EXAMPLES_SOURCES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} examples/*.cpp)

# Для каждого создаём отдельную цель
foreach(example_src ${EXAMPLES_SOURCES})
    get_filename_component(example_name ${example_src} NAME_WE)

    add_executable(${example_name} ${example_src})

    target_include_directories(${example_name} PRIVATE ${PROJECT_INCLUDE_DIRS})
    
    if(COMMON_WARN_FLAGS)
        target_compile_options(${example_name} PRIVATE ${COMMON_WARN_FLAGS})
    endif()

    target_link_directories(${example_name} PRIVATE ${PROJECT_LIBRARY_DIRS})
    target_compile_definitions(${example_name} PRIVATE ${PROJECT_DEFINES})
    target_link_libraries(${example_name} PRIVATE ${PROJECT_LIBS})

    # Добавляем заголовки как SOURCES (чтобы IDE показывала их)
    set_source_files_properties(${PROJECT_HEADERS} PROPERTIES HEADER_FILE_ONLY ON)
    target_sources(${example_name} PRIVATE ${PROJECT_HEADERS})
    
    foreach(dll ${DLL_FILES})
        add_custom_command(TARGET ${example_name} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${dll}"
            "$<TARGET_FILE_DIR:${example_name}>"
        )
    endforeach()
endforeach()
