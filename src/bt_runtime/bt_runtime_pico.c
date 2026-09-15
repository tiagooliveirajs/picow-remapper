#include "remapper/bt_runtime/bt_runtime.h"

#include "btstack.h"
#include "g06_hog_host.h"
#include "pico/cyw43_arch.h"

static remapper_bt_runtime_session_setup_fn g_session_setup;
static bool g_started;

bool remapper_bt_runtime_start(remapper_bt_runtime_session_setup_fn session_setup)
{
    if (g_started || session_setup == NULL) return false;

    remapper_bt_runtime_reset();
    g_session_setup = session_setup;

    /*
     * The canonical G06 runtime uses pico_cyw43_arch_threadsafe_background.
     * Bluetooth/CYW43 callbacks therefore run from the SDK's low-priority
     * async-context IRQ and do not need a dedicated second core or a blocking
     * application-owned Bluetooth run loop.
     *
     * Keeping cyw43_arch_init() on core 0 follows the Pico SDK Bluetooth
     * examples and avoids two failure modes seen in the earlier G06 runtime:
     * a 2 KiB default Core1 stack under deep pairing/GATT call chains, and
     * CYW43 async-context hangs when initialized from Core1.
     */
    if (cyw43_arch_init() != 0) {
        g_session_setup = NULL;
        return false;
    }

    l2cap_init();
    sm_init();
    sm_set_io_capabilities(IO_CAPABILITY_NO_INPUT_NO_OUTPUT);
    sm_set_authentication_requirements(
        SM_AUTHREQ_SECURE_CONNECTION | SM_AUTHREQ_BONDING);
    gatt_client_init();

    /*
     * Some BLE peripherals issue ATT queries to the central. A minimal local
     * GAP service matches the proven HOGP host POC and keeps those peers
     * interoperable without exposing any product state through GATT.
     */
    att_server_init(profile_data, NULL, NULL);

    g_session_setup();
    hci_power_control(HCI_POWER_ON);

    g_started = true;
    return true;
}
