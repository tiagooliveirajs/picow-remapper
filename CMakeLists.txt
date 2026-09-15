cmake_minimum_required(VERSION 3.16)

project(picow_remapper LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

option(REMAPPER_WARNINGS_AS_ERRORS "Treat compiler warnings as errors" ON)

add_library(domain INTERFACE)
target_include_directories(domain INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/src/domain/include
)

add_library(hid_aggregator STATIC
    src/hid_aggregator/hid_aggregator.c
)
target_include_directories(hid_aggregator PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src/hid_aggregator/include
)
target_link_libraries(hid_aggregator PUBLIC domain)

if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(hid_aggregator PRIVATE
        -Wall
        -Wextra
        -Wpedantic
        $<$<BOOL:${REMAPPER_WARNINGS_AS_ERRORS}>:-Werror>
    )
endif()

enable_testing()

add_executable(test_hid_aggregator
    tests/test_hid_aggregator.c
)
target_link_libraries(test_hid_aggregator PRIVATE hid_aggregator)
if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(test_hid_aggregator PRIVATE
        -Wall
        -Wextra
        -Wpedantic
        $<$<BOOL:${REMAPPER_WARNINGS_AS_ERRORS}>:-Werror>
    )
endif()
add_test(NAME canonical_hid_ownership COMMAND test_hid_aggregator)

find_package(Python3 REQUIRED COMPONENTS Interpreter)
add_test(
    NAME architecture_boundaries
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/test_architecture.py ${CMAKE_CURRENT_SOURCE_DIR}
)
