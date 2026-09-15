from __future__ import annotations

import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
MODULE_FILE = ROOT / "cmake" / "RemapperModules.cmake"
ROOT_CMAKE = ROOT / "CMakeLists.txt"
SRC = ROOT / "src"
USB_SRC = SRC / "usb_hid"
BT_RUNTIME_SRC = SRC / "bt_runtime"
BLE_HOGP_SRC = SRC / "ble_hogp"
DOMAIN_HID = SRC / "domain" / "include" / "remapper" / "domain" / "hid.h"
AGG_SRC = SRC / "hid_aggregator"
PROFILE_SRC = SRC / "profiles"
REMAP_SRC = SRC / "remap"
HIDPP_SRC = SRC / "logitech_hidpp"
STORAGE_SRC = SRC / "storage_pico"

EXPECTED_MODULES = {
    "domain", "ux_model", "interaction", "renderer", "hat", "device_registry",
    "profiles", "remap", "hid_aggregator", "usb_hid", "bt_runtime", "ble_hogp",
    "classic_hid", "logitech_hidpp", "connection_coordinator", "storage_pico", "app",
}

EXPECTED_DEPS = {
    "ux_model": {"domain"},
    "interaction": {"ux_model", "domain"},
    "device_registry": {"domain"},
    "profiles": {"domain"},
    "remap": {"domain"},
    "hid_aggregator": {"domain"},
    "ble_hogp": {"bt_runtime", "domain"},
    "classic_hid": {"bt_runtime", "domain"},
    "logitech_hidpp": {"domain"},
    "connection_coordinator": {"device_registry", "domain"},
}

FORBIDDEN_MACRO_PREFIXES = (
    "tud_", "tusb_", "hid_host_", "btstack_", "hci_", "sm_", "cyw43_", "gpio_", "spi_",
)


def fail(message: str) -> None:
    raise AssertionError(message)


def parse_deps(cmake_text: str, target: str) -> set[str]:
    match = re.search(
        rf"target_link_libraries\(\s*{re.escape(target)}\s+(?:INTERFACE|PUBLIC)\s+(.*?)\)",
        cmake_text,
        flags=re.DOTALL,
    )
    if not match:
        return set()
    return set(re.findall(r"\b[a-z][a-z0-9_]*\b", match.group(1)))


def under(path: Path, root: Path) -> bool:
    return path == root or path.is_relative_to(root)


def main() -> None:
    module_text = MODULE_FILE.read_text(encoding="utf-8")
    root_cmake = ROOT_CMAKE.read_text(encoding="utf-8")

    declared = set(re.findall(r"remapper_add_module\(\s*([a-z][a-z0-9_]*)\b", module_text))
    if declared != EXPECTED_MODULES:
        fail(f"canonical CMake module set mismatch: {sorted(declared)}")

    for target, expected in EXPECTED_DEPS.items():
        actual = parse_deps(module_text, target)
        if actual != expected:
            fail(f"{target} dependencies {sorted(actual)} != {sorted(expected)}")

    for target in {"domain", "renderer", "hat", "usb_hid", "bt_runtime", "storage_pico"}:
        if parse_deps(module_text, target):
            fail(f"{target} has a forbidden canonical dependency")

    for target in (
        "ux_model", "interaction", "renderer", "hat", "profiles", "remap",
        "hid_aggregator", "usb_hid", "bt_runtime", "ble_hogp", "logitech_hidpp",
        "storage_pico", "app",
    ):
        if not re.search(rf"remapper_add_module\(\s*{target}\s+STATIC\b", module_text):
            fail(f"{target} is not a compiled library in G07")

    required_files = (
        DOMAIN_HID,
        AGG_SRC / "hid_aggregator.c",
        BT_RUNTIME_SRC / "bt_runtime.c",
        BT_RUNTIME_SRC / "bt_runtime_pico.c",
        BLE_HOGP_SRC / "ble_hogp.c",
        BLE_HOGP_SRC / "ble_hogp_pico.c",
        BLE_HOGP_SRC / "ble_hogp_framing.c",
        BLE_HOGP_SRC / "ble_hogp_runtime_g07.c",
        PROFILE_SRC / "profiles.c",
        REMAP_SRC / "remap.c",
        HIDPP_SRC / "logitech_hidpp.c",
        HIDPP_SRC / "logitech_hidpp_pico.c",
        STORAGE_SRC / "storage_pico.c",
    )
    for path in required_files:
        if not path.is_file():
            fail(f"required G07 implementation missing: {path.relative_to(ROOT)}")

    if 'set(PICO_BOARD "pico2_w"' not in root_cmake:
        fail("canonical Pico board is not fixed to pico2_w")
    if "pico_add_extra_outputs(picow_remapper)" not in root_cmake:
        fail("Pico build does not generate UF2/extra outputs")
    if 'REMAPPER_GATE_NAME="REMAPPER-G07"' not in root_cmake:
        fail("firmware gate marker is not REMAPPER-G07")
    for token in (
        "src/renderer/st7789_pico.c",
        "src/hat/hat_pico.c",
        "src/usb_hid/usb_hid_pico.c",
        "src/usb_hid/usb_descriptors.c",
        "src/bt_runtime/bt_runtime_pico.c",
        "src/ble_hogp/ble_hogp_pico.c",
        "src/logitech_hidpp/logitech_hidpp_pico.c",
        "tinyusb_device",
        "pico_btstack_ble",
        "pico_btstack_cyw43",
        "pico_cyw43_arch_threadsafe_background",
        "PICO_BTSTACK_CYW43_MAX_HCI_PROCESS_LOOP_COUNT=8",
        "REMAPPER_PICO_STORAGE=1",
        "pico_flash",
    ):
        if token not in root_cmake:
            fail(f"G07 Pico composition is missing {token}")
    if "pico_multicore" in root_cmake:
        fail("G07 Bluetooth runtime must not depend on pico_multicore")
    if "CFG_TUD_CDC=1" not in root_cmake or "tinyusb_device_base" not in root_cmake:
        fail("G07 debug CDC is not forced into the TinyUSB interface sources")

    source_files = [
        path for path in SRC.rglob("*")
        if path.is_file() and path.suffix.lower() in {".c", ".h"}
    ]
    include_c = re.compile(r"#\s*include\s*[<\"][^>\"]+\.c[>\"]")
    define_macro = re.compile(r"#\s*define\s+([A-Za-z_][A-Za-z0-9_]*)")

    for path in source_files:
        text = path.read_text(encoding="utf-8")
        lowered = text.lower()
        if include_c.search(text):
            fail(f"textual .c include is forbidden: {path.relative_to(ROOT)}")
        if "picow-mouse-remapper" in text:
            fail(f"legacy repository structural dependency: {path.relative_to(ROOT)}")
        for macro in define_macro.findall(text):
            if macro.startswith(FORBIDDEN_MACRO_PREFIXES):
                fail(f"macro interception is forbidden: {macro} in {path.relative_to(ROOT)}")

        if not under(path, USB_SRC):
            if '"tusb.h"' in lowered or "tud_hid_" in lowered or "tud_descriptor_" in lowered:
                fail(f"TinyUSB leaked outside usb_hid: {path.relative_to(ROOT)}")

        if not under(path, BT_RUNTIME_SRC) and not under(path, BLE_HOGP_SRC):
            for token in (
                '"btstack.h"', "hids_client_", "hci_power_control", "gap_start_scan",
                "gap_connect(", "sm_request_pairing", "cyw43_arch_init", "multicore_launch_core1",
            ):
                if token in lowered:
                    fail(f"Bluetooth runtime leaked outside bt_runtime/ble_hogp: {token} in {path.relative_to(ROOT)}")

    canonical_text = DOMAIN_HID.read_text(encoding="utf-8") + "\n" + "\n".join(
        path.read_text(encoding="utf-8")
        for path in AGG_SRC.rglob("*")
        if path.is_file() and path.suffix.lower() in {".c", ".h"}
    )
    canonical_lower = canonical_text.lower()
    for token in (
        "btstack", "cyw43", "tinyusb", "tusb.h", "tud_", "hid_host", "hardware/gpio",
        "hardware/spi", "pico/stdlib", "report_id", "descriptor_report", "remote_report",
    ):
        if token in canonical_lower:
            fail(f"canonical HID boundary contains forbidden transport/layout token: {token}")

    for root, name in ((PROFILE_SRC, "profiles"), (REMAP_SRC, "remap")):
        text = "\n".join(
            p.read_text(encoding="utf-8") for p in root.rglob("*")
            if p.is_file() and p.suffix.lower() in {".c", ".h"}
        ).lower()
        for token in ("btstack", "cyw43", "tinyusb", "tud_", "hardware/flash", "pico/"):
            if token in text:
                fail(f"{name} is not host-pure: {token}")

    hidpp_common = (HIDPP_SRC / "logitech_hidpp.c").read_text(encoding="utf-8").lower()
    hidpp_header = (
        HIDPP_SRC / "include" / "remapper" / "logitech_hidpp" / "logitech_hidpp.h"
    ).read_text(encoding="utf-8").lower()
    for token in ("btstack", "hids_client", "tinyusb", "renderer", "hardware/"):
        if token in hidpp_common:
            fail(f"HID++ common backend leaked platform dependency: {token}")
    for token in ("0x1b04", "0x0056", "0x03u", "0x02u"):
        if token not in hidpp_common and token not in hidpp_header:
            fail(f"G07 HID++ Forward contract missing {token}")

    ble_text = "\n".join(
        path.read_text(encoding="utf-8")
        for path in BLE_HOGP_SRC.rglob("*")
        if path.is_file() and path.suffix.lower() in {".c", ".h"}
    ).lower()
    for token in (
        "tinyusb", "tud_", "renderer", "st7789", "remapper/hat",
        "remapper/profiles", "storage_pico",
    ):
        if token in ble_text:
            fail(f"ble_hogp crosses a product/storage/UI boundary: {token}")
    for token in (
        "hids_client_init", "hids_client_connect", "gap_start_scan",
        "parser_configure", "vendor_backend",
    ):
        if token not in ble_text:
            fail(f"G07 BLE HOGP adapter is missing {token}")

    bt_runtime_text = "\n".join(
        path.read_text(encoding="utf-8")
        for path in BT_RUNTIME_SRC.rglob("*")
        if path.is_file() and path.suffix.lower() in {".c", ".h"}
    ).lower()
    for token in ("remapper/domain", "renderer", "st7789", "remapper/hat", "usb_hid", "tud_"):
        if token in bt_runtime_text:
            fail(f"bt_runtime crosses a product/UI/USB boundary: {token}")
    for token in ("cyw43_arch_init", "hci_power_control"):
        if token not in bt_runtime_text:
            fail(f"bt_runtime does not own required lifecycle primitive: {token}")
    for token in ("multicore_launch_core1", "btstack_run_loop_execute"):
        if token in bt_runtime_text:
            fail(f"background Bluetooth runtime must not use {token}")

    usb_text = "\n".join(
        path.read_text(encoding="utf-8")
        for path in USB_SRC.rglob("*")
        if path.is_file() and path.suffix.lower() in {".c", ".h"}
    ).lower()
    for token in ("btstack", "cyw43", "ble_hogp", "classic_hid", "hid_host"):
        if token in usb_text:
            fail(f"fixed USB identity depends on Bluetooth token: {token}")
    for token in ("tud_disconnect", "tud_connect"):
        if token in usb_text:
            fail(f"G07 must never force USB re-enumeration: {token}")

    usb_pico = (USB_SRC / "usb_hid_pico.c").read_text(encoding="utf-8")
    if "tud_task_ext(0u, false)" not in usb_pico:
        fail("USB service must remain explicitly non-blocking")
    if "REMAPPER_USB_DEBUG_FLUSH_BUDGET" not in usb_pico:
        fail("debug CDC flush must have a per-tick byte budget")

    main_text = (SRC / "app" / "main.c").read_text(encoding="utf-8").lower()
    for token in ("tinyusb", "tusb", "tud_", "btstack", "cyw43", "hid_host", "gpio_", "spi_"):
        if token in main_text:
            fail(f"Core0 app composition crosses a transport/HAL boundary: {token}")
    for token in (
        "remapper_bt_runtime_poll", "remapper_remap_process_mouse",
        "remapper_hid_aggregator_apply_mouse", "remapper_hid_aggregator_apply_keyboard",
        "remapper_usb_hid_pico_send_mouse", "remapper_usb_hid_pico_send_keyboard",
        "remapper_profiles_draft_set", "remapper_profiles_custom_candidate",
        "remapper_storage_pico_write", "remapper_logitech_hidpp_pico_set_forward_fix",
    ):
        if token not in main_text:
            fail(f"G07 Core0 composition is missing {token}")
    if "remapper_runtime_messages_per_tick" not in main_text:
        fail("Core0 must bound Bluetooth queue work per UI tick")


if __name__ == "__main__":
    main()
