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
    target_include_directories(
        domain
        INTERFACE
            ${CMAKE_CURRENT_FUNCTION_LIST_DIR}/../src/domain/include
    )

    remapper_add_module(
        ux_model
        STATIC
        ${CMAKE_CURRENT_FUNCTION_LIST_DIR}/../src/ux_model/ux_model.c
    )
    target_include_directories(
        ux_model
        PUBLIC
            ${CMAKE_CURRENT_FUNCTION_LIST_DIR}/../src/ux_model/include
    )
    target_link_libraries(ux_model PUBLIC domain)

    remapper_add_module(
        interaction
        STATIC
        ${CMAKE_CURRENT_FUNCTION_LIST_DIR}/../src/interaction/interaction.c
    )
    target_include_directories(
        interaction
        PUBLIC
            ${CMAKE_CURRENT_FUNCTION_LIST_DIR}/../src/interaction/include
    )
    target_link_libraries(interaction PUBLIC ux_model domain)

    remapper_add_module(renderer INTERFACE)
    remapper_add_module(hat INTERFACE)

    remapper_add_module(device_registry INTERFACE)
    target_link_libraries(device_registry INTERFACE domain)

    remapper_add_module(profiles INTERFACE)
    target_link_libraries(profiles INTERFACE domain)

    remapper_add_module(remap INTERFACE)
    target_link_libraries(remap INTERFACE domain)

    remapper_add_module(hid_aggregator INTERFACE)
    target_link_libraries(hid_aggregator INTERFACE domain)

    remapper_add_module(usb_hid INTERFACE)
    remapper_add_module(bt_runtime INTERFACE)

    remapper_add_module(ble_hogp INTERFACE)
    target_link_libraries(ble_hogp INTERFACE bt_runtime domain)

    remapper_add_module(classic_hid INTERFACE)
    target_link_libraries(classic_hid INTERFACE bt_runtime domain)

    remapper_add_module(logitech_hidpp INTERFACE)
    target_link_libraries(logitech_hidpp INTERFACE domain)

    remapper_add_module(connection_coordinator INTERFACE)
    target_link_libraries(connection_coordinator INTERFACE device_registry domain)

    remapper_add_module(storage_pico INTERFACE)

    remapper_add_module(app INTERFACE)
    target_link_libraries(
        app
        INTERFACE
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
