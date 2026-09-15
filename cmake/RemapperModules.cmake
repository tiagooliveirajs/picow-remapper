function(remapper_add_module name)
    add_library(${name} INTERFACE)
    add_library(remapper::${name} ALIAS ${name})
    target_compile_features(${name} INTERFACE c_std_11)
endfunction()

function(remapper_define_modules)
    remapper_add_module(domain)
    target_include_directories(
        domain
        INTERFACE
            ${CMAKE_CURRENT_FUNCTION_LIST_DIR}/../src/domain/include
    )

    remapper_add_module(ux_model)
    target_link_libraries(ux_model INTERFACE domain)

    remapper_add_module(interaction)
    target_link_libraries(interaction INTERFACE ux_model domain)

    remapper_add_module(renderer)
    remapper_add_module(hat)

    remapper_add_module(device_registry)
    target_link_libraries(device_registry INTERFACE domain)

    remapper_add_module(profiles)
    target_link_libraries(profiles INTERFACE domain)

    remapper_add_module(remap)
    target_link_libraries(remap INTERFACE domain)

    remapper_add_module(hid_aggregator)
    target_link_libraries(hid_aggregator INTERFACE domain)

    remapper_add_module(usb_hid)
    remapper_add_module(bt_runtime)

    remapper_add_module(ble_hogp)
    target_link_libraries(ble_hogp INTERFACE bt_runtime domain)

    remapper_add_module(classic_hid)
    target_link_libraries(classic_hid INTERFACE bt_runtime domain)

    remapper_add_module(logitech_hidpp)
    target_link_libraries(logitech_hidpp INTERFACE domain)

    remapper_add_module(connection_coordinator)
    target_link_libraries(connection_coordinator INTERFACE device_registry domain)

    remapper_add_module(storage_pico)

    remapper_add_module(app)
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
