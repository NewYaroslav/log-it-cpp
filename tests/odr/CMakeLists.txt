set(ODR_FIXTURE_SOURCES
    a.cpp
    b.cpp
)

add_executable(logit_odr_cxx11 ${ODR_FIXTURE_SOURCES})
target_link_libraries(logit_odr_cxx11 PRIVATE log-it-cpp)
set_target_properties(logit_odr_cxx11 PROPERTIES
    CXX_STANDARD 11
    CXX_STANDARD_REQUIRED ON
)
add_test(NAME logit_odr_cxx11 COMMAND logit_odr_cxx11)

add_executable(logit_odr_cxx17 ${ODR_FIXTURE_SOURCES})
target_link_libraries(logit_odr_cxx17 PRIVATE log-it-cpp)
set_target_properties(logit_odr_cxx17 PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
)
add_test(NAME logit_odr_cxx17 COMMAND logit_odr_cxx17)
