function(remapper_add_module name kind)
    if(kind STREQUAL "INTERFACE")
        add_library(${name} INTERFACE)
        add_library(remapper::${name} ALIAS ${name})
        target_compile_features(${name} INTERFACE c_std_11)
    elseif(kind STREQUAL "STATIC")
        add_library(${name} STATIC ${ARGN})
        add_library(remapper::${name} ALIAS ${name})
        target_compile_features(${name} PUBLIC c_std_11)
    else()
        message(FATAL_ERROR "Unknown remapper module kind '${kind}' for ${name}")
    endif()
endfunction()

function(remapper_define_modules)
    remapper_add_module(domain INTERFACE)
    target_include_directories(domain INTERFACE ${CMAKE_CURRENT_FUNCTION_LIST_DIR}/../src/domain/include)

    remapper_add_module(ux_model STATIC ${CMAKE_CURRENT_FUNCTION_LIST_DIR}/../src/ux_model/ux_model.c)
    target_include_directories(ux_model PUBLIC ${CMAKE_CURRENT_FUNCTION_LIST_DIR}/../src/ux_model/include)
    target_link_libraries(ux_model PUBLIC domain)

    remapper_add_module(interaction STATIC ${CMAKE_CURRENT_FUNCTION_LIST_DIR}/../src/interaction/interaction.c)
    target_include_directories(interaction PUBLIC ${CMAKE_CURRENT_FUNCTION_LIST_DIR}/../src/interaction/include)
    target_link_libraries(interaction PUBLIC ux_model domain)

    remapper_add_module(renderer STATIC ${CMAKE_CURRENT_FUNCTION_LIST_DIR}/../src/renderer/renderer.c)
    target_include_directories(renderer PUBLIC ${CMAKE_CURRENT_FUNCTION_LIST_DIR}/../src/renderer/include)

    remapper_add_module(hat STATIC ${CMAKE_CURRENT_FUNCTION_LIST_DIR}/../src/hat/hat.c)
    target_include_directories(hat PUBLIC ${CMAKE_CURRENT_FUNCTION_LIST_DIR}/../src/hat/include)

    remapper_add_module(device_registry INTERFACE)
    target_link_libraries(device_registry INTERFACE domain)

    remapper_add_module(profiles INTERFACE)
    target_link_libraries(profiles INTERFACE domain)

    remapper_add_module(remap INTERFACE)
    target_link_libraries(remap INTERFACE domain)

    remapper_add_module(hid_aggregator STATIC ${CMAKE_CURRENT_FUNCTION_LIST_DIR}/../src/hid_aggregator/hid_aggregator.c)
    target_include_directories(hid_aggregator PUBLIC ${CMAKE_CURRENT_FUNCTION_LIST_DIR}/../src/hid_aggregator/include)
    target_link_libraries(hid_aggregator PUBLIC domain)

    remapper_add_module(usb_hid STATIC ${CMAKE_CURRENT_FUNCTION_LIST_DIR}/../src/usb_hid/usb_hid.c)
    target_include_directories(usb_hid PUBLIC ${CMAKE_CURRENT_FUNCTION_LIST_DIR}/../src/usb_hid/include)

    remapper_add_module(bt_runtime STATIC ${CMAKE_CURRENT_FUNCTION_LIST_DIR}/../src/bt_runtime/bt_runtime.c)
    target_include_directories(bt_runtime PUBLIC ${CMAKE_CURRENT_FUNCTION_LIST_DIR}/../src/bt_runtime/include)

    remapper_add_module(ble_hogp STATIC ${CMAKE_CURRENT_FUNCTION_LIST_DIR}/../src/ble_hogp/ble_hogp.c)
    target_include_directories(ble_hogp PUBLIC ${CMAKE_CURRENT_FUNCTION_LIST_DIR}/../src/ble_hogp/include)
    target_link_libraries(ble_hogp PUBLIC bt_runtime domain)

    remapper_add_module(classic_hid INTERFACE)
    target_link_libraries(classic_hid INTERFACE bt_runtime domain)

    remapper_add_module(logitech_hidpp INTERFACE)
    target_link_libraries(logitech_hidpp INTERFACE domain)

    remapper_add_module(connection_coordinator INTERFACE)
    target_link_libraries(connection_coordinator INTERFACE device_registry domain)

    remapper_add_module(storage_pico INTERFACE)

    remapper_add_module(app STATIC ${CMAKE_CURRENT_FUNCTION_LIST_DIR}/../src/app/ui_projection.c)
    target_include_directories(app PUBLIC ${CMAKE_CURRENT_FUNCTION_LIST_DIR}/../src/app/include)
    target_link_libraries(
        app
        PUBLIC
            domain
            ux_model
            interaction
            renderer
            hat
            device_registry
            profiles
            remap
            hid_aggregator
            usb_hid
            bt_runtime
            ble_hogp
            classic_hid
            logitech_hidpp
            connection_coordinator
            storage_pico
    )
endfunction()
