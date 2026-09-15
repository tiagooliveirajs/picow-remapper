cmake_minimum_required(VERSION 3.20)

option(REMAPPER_BUILD_PICO "Build the Raspberry Pi Pico 2 W firmware scaffold" OFF)
option(REMAPPER_BUILD_TESTS "Build host-side bootstrap, architecture and UX tests" ON)

if(REMAPPER_BUILD_PICO)
    set(PICO_BOARD "pico2_w" CACHE STRING "Raspberry Pi Pico SDK board target")
    if(NOT PICO_BOARD STREQUAL "pico2_w")
        message(FATAL_ERROR "PicoW Remapper canonical target is Pico 2 W (PICO_BOARD=pico2_w)")
    endif()

    if(NOT DEFINED PICO_SDK_PATH)
        if(DEFINED ENV{PICO_SDK_PATH})
            set(PICO_SDK_PATH "$ENV{PICO_SDK_PATH}")
        else()
            message(FATAL_ERROR "REMAPPER_BUILD_PICO requires PICO_SDK_PATH")
        endif()
    endif()

    include("${PICO_SDK_PATH}/external/pico_sdk_import.cmake")
    project(picow_remapper LANGUAGES C CXX ASM)
else()
    project(picow_remapper LANGUAGES C)
endif()

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
if(REMAPPER_BUILD_PICO)
    # Pico SDK RP2350 sources use GNU C extensions such as the `asm` keyword.
    set(CMAKE_C_EXTENSIONS ON)
else()
    # Host-side domain/UX/architecture tests remain strict ISO C11.
    set(CMAKE_C_EXTENSIONS OFF)
endif()

include(cmake/RemapperModules.cmake)
remapper_define_modules()

if(REMAPPER_BUILD_TESTS)
    enable_testing()
    find_package(Python3 REQUIRED COMPONENTS Interpreter)

    add_executable(test_scaffold tests/test_scaffold.c)
    target_link_libraries(test_scaffold PRIVATE domain)
    target_compile_options(test_scaffold PRIVATE -Wall -Wextra -Wpedantic -Werror)

    add_executable(test_ux_interaction tests/test_ux_interaction.c)
    target_link_libraries(test_ux_interaction PRIVATE interaction)
    target_compile_options(test_ux_interaction PRIVATE -Wall -Wextra -Wpedantic -Werror)

    add_test(NAME scaffold_contract COMMAND test_scaffold)
    add_test(NAME ux_interaction_contract COMMAND test_ux_interaction)
    add_test(
        NAME architecture_boundaries
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/test_architecture.py
    )
endif()

if(REMAPPER_BUILD_PICO)
    pico_sdk_init()

    add_executable(picow_remapper src/app/main.c)
    target_link_libraries(picow_remapper PRIVATE pico_stdlib app)
    target_compile_definitions(
        picow_remapper
        PRIVATE
            REMAPPER_GATE_NAME="REMAPPER-G02"
            REMAPPER_FIRMWARE_VERSION="0.2.0-g02"
    )

    # G02 adds only hardware-neutral UX state/interaction logic.
    # USB, Bluetooth, ST7789 and HAT GPIO behavior remain later gates.
    pico_enable_stdio_usb(picow_remapper 0)
    pico_enable_stdio_uart(picow_remapper 0)
    pico_add_extra_outputs(picow_remapper)
endif()
