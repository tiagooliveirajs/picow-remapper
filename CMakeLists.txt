cmake_minimum_required(VERSION 3.20)
option(REMAPPER_BUILD_PICO "Build the Raspberry Pi Pico 2 W firmware" OFF)
option(REMAPPER_BUILD_TESTS "Build host-side architecture, UX, USB, HID and BLE-parser tests" ON)

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
    set(CMAKE_C_EXTENSIONS ON)
    pico_sdk_init()
else()
    set(CMAKE_C_EXTENSIONS OFF)
endif()

include(cmake/RemapperModules.cmake)
remapper_define_modules()

if(REMAPPER_BUILD_PICO)
    target_sources(renderer PRIVATE src/renderer/st7789_pico.c)
    target_link_libraries(renderer PRIVATE pico_stdlib hardware_gpio hardware_spi)

    target_sources(hat PRIVATE src/hat/hat_pico.c)
    target_link_libraries(hat PRIVATE pico_stdlib hardware_gpio)

    target_sources(
        usb_hid
        PRIVATE
            src/usb_hid/usb_hid_pico.c
            src/usb_hid/usb_descriptors.c
    )
    target_link_libraries(usb_hid PUBLIC tinyusb_device)

    target_sources(bt_runtime PRIVATE src/bt_runtime/bt_runtime_pico.c)
    target_link_libraries(
        bt_runtime
        PRIVATE
            pico_stdlib
            pico_multicore
            pico_btstack_ble
            pico_btstack_cyw43
            pico_cyw43_arch_threadsafe_background
    )
    target_compile_definitions(bt_runtime PUBLIC CYW43_LWIP=0)
    pico_btstack_make_gatt_header(
        bt_runtime
        PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}/src/bt_runtime/g06_hog_host.gatt
    )

    target_sources(ble_hogp PRIVATE src/ble_hogp/ble_hogp_pico.c)
    target_link_libraries(ble_hogp PRIVATE pico_stdlib pico_btstack_ble)
endif()

if(REMAPPER_BUILD_TESTS)
    enable_testing()
    find_package(Python3 REQUIRED COMPONENTS Interpreter)

    add_executable(test_scaffold tests/test_scaffold.c)
    target_link_libraries(test_scaffold PRIVATE domain)
    target_compile_options(test_scaffold PRIVATE -Wall -Wextra -Wpedantic -Werror)

    add_executable(test_ux_interaction tests/test_ux_interaction.c)
    target_link_libraries(test_ux_interaction PRIVATE interaction)
    target_compile_options(test_ux_interaction PRIVATE -Wall -Wextra -Wpedantic -Werror)

    add_executable(test_g03_ui tests/test_g03_ui.c)
    target_link_libraries(test_g03_ui PRIVATE app)
    target_compile_options(test_g03_ui PRIVATE -Wall -Wextra -Wpedantic -Werror)

    add_executable(test_g04_usb_layout tests/test_g04_usb_layout.c)
    target_link_libraries(test_g04_usb_layout PRIVATE renderer usb_hid)
    target_compile_options(test_g04_usb_layout PRIVATE -Wall -Wextra -Wpedantic -Werror)

    add_executable(test_g05_hid_ownership tests/test_g05_hid_ownership.c)
    target_link_libraries(test_g05_hid_ownership PRIVATE hid_aggregator)
    target_compile_options(test_g05_hid_ownership PRIVATE -Wall -Wextra -Wpedantic -Werror)

    add_executable(test_g06_ble_mouse tests/test_g06_ble_mouse.c)
    target_link_libraries(test_g06_ble_mouse PRIVATE ble_hogp bt_runtime hid_aggregator usb_hid)
    target_compile_options(test_g06_ble_mouse PRIVATE -Wall -Wextra -Wpedantic -Werror)

    add_test(NAME scaffold_contract COMMAND test_scaffold)
    add_test(NAME ux_interaction_contract COMMAND test_ux_interaction)
    add_test(NAME g03_renderer_hat_contract COMMAND test_g03_ui)
    add_test(NAME g04_usb_layout_contract COMMAND test_g04_usb_layout)
    add_test(NAME g05_canonical_hid_ownership COMMAND test_g05_hid_ownership)
    add_test(NAME g06_ble_mouse_passthrough COMMAND test_g06_ble_mouse)
    add_test(NAME architecture_boundaries COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/test_architecture.py)
endif()

if(REMAPPER_BUILD_PICO)
    add_executable(picow_remapper src/app/main.c)
    target_link_libraries(picow_remapper PRIVATE pico_stdlib app usb_hid)
    target_compile_definitions(
        picow_remapper
        PRIVATE
            REMAPPER_GATE_NAME="REMAPPER-G06"
            REMAPPER_FIRMWARE_VERSION="0.6.1-g06-uart"
            CYW43_LWIP=0
    )
    pico_enable_stdio_usb(picow_remapper 0)
    pico_enable_stdio_uart(picow_remapper 1)
    pico_add_extra_outputs(picow_remapper)
endif()
