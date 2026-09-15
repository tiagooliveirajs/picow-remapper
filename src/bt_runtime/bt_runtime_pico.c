#include "remapper/bt_runtime/bt_runtime.h"

#include "btstack.h"
#include "g06_hog_host.gatt.h"
#include "pico/cyw43_arch.h"
#include "pico/multicore.h"

static remapper_bt_runtime_session_setup_fn g_session_setup;
static bool g_started;

static void remapper_bt_runtime_core1_main(void)
{
    if (cyw43_arch_init() != 0) return;

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

    if (g_session_setup != NULL) g_session_setup();

    hci_power_control(HCI_POWER_ON);
    btstack_run_loop_execute();
}

bool remapper_bt_runtime_start(remapper_bt_runtime_session_setup_fn session_setup)
{
    if (g_started || session_setup == NULL) return false;

    remapper_bt_runtime_reset();
    g_session_setup = session_setup;
    g_started = true;
    multicore_launch_core1(remapper_bt_runtime_core1_main);
    return true;
}
