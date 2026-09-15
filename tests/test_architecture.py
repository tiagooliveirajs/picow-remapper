from __future__ import annotations

import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
MODULE_FILE = ROOT / "cmake" / "RemapperModules.cmake"
ROOT_CMAKE = ROOT / "CMakeLists.txt"
SRC = ROOT / "src"
USB_SRC = SRC / "usb_hid"
DOMAIN_HID = SRC / "domain" / "include" / "remapper" / "domain" / "hid.h"
AGG_SRC = SRC / "hid_aggregator"

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

    for target in ("ux_model", "interaction", "renderer", "hat", "hid_aggregator", "usb_hid", "app"):
        if not re.search(rf"remapper_add_module\(\s*{target}\s+STATIC\b", module_text):
            fail(f"{target} is not a compiled library in G05")

    if not DOMAIN_HID.is_file():
        fail("G05 canonical HID domain header is missing")
    if not (AGG_SRC / "hid_aggregator.c").is_file():
        fail("G05 HID ownership aggregator implementation is missing")
    if not (AGG_SRC / "include" / "remapper" / "hid_aggregator" / "hid_aggregator.h").is_file():
        fail("G05 HID ownership aggregator public header is missing")

    if 'set(PICO_BOARD "pico2_w"' not in root_cmake:
        fail("canonical Pico board is not fixed to pico2_w")
    if "pico_add_extra_outputs(picow_remapper)" not in root_cmake:
        fail("Pico build does not generate UF2/extra outputs")
    if 'REMAPPER_GATE_NAME="REMAPPER-G05"' not in root_cmake:
        fail("firmware gate marker is not REMAPPER-G05")
    if "src/renderer/st7789_pico.c" not in root_cmake:
        fail("G05 regressed the ST7789 Pico display adapter")
    if "src/hat/hat_pico.c" not in root_cmake:
        fail("G05 regressed the Pico HAT GPIO adapter")
    if "src/usb_hid/usb_hid_pico.c" not in root_cmake or "src/usb_hid/usb_descriptors.c" not in root_cmake:
        fail("G05 regressed the fixed TinyUSB adapter/descriptors")
    if "tinyusb_device" not in root_cmake:
        fail("G05 usb_hid target is not linked to TinyUSB device support")

    source_files = [
        path for path in SRC.rglob("*")
        if path.is_file() and path.suffix.lower() in {".c", ".h"}
    ]
    include_c = re.compile(r"#\s*include\s*[<\"][^>\"]+\.c[>\"]")
    define_macro = re.compile(r"#\s*define\s+([A-Za-z_][A-Za-z0-9_]*)")

    for path in source_files:
        text = path.read_text(encoding="utf-8")
        if include_c.search(text):
            fail(f"textual .c include is forbidden: {path.relative_to(ROOT)}")
        if "picow-mouse-remapper" in text:
            fail(f"legacy repository structural dependency: {path.relative_to(ROOT)}")
        for macro in define_macro.findall(text):
            if macro.startswith(FORBIDDEN_MACRO_PREFIXES):
                fail(f"macro interception is forbidden: {macro} in {path.relative_to(ROOT)}")

        if not path.is_relative_to(USB_SRC):
            lowered = text.lower()
            if '"tusb.h"' in lowered or "tud_hid_" in lowered or "tud_descriptor_" in lowered:
                fail(f"TinyUSB leaked outside usb_hid: {path.relative_to(ROOT)}")

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
            fail(f"G05 canonical HID boundary contains forbidden transport/layout token: {token}")

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
            fail(f"G05 must never force USB re-enumeration: {token}")

    tusb_config = (USB_SRC / "include" / "tusb_config.h").read_text(encoding="utf-8")
    if not re.search(r"#define\s+CFG_TUD_HID\s+2\b", tusb_config):
        fail("TinyUSB is not configured for exactly two HID interfaces")
    descriptor_text = (USB_SRC / "usb_descriptors.c").read_text(encoding="utf-8")
    for token in ("HID_ITF_PROTOCOL_MOUSE", "HID_ITF_PROTOCOL_KEYBOARD", "EPNUM_MOUSE", "EPNUM_KEYBOARD"):
        if token not in descriptor_text:
            fail(f"fixed USB descriptor is missing {token}")

    main_text = (SRC / "app" / "main.c").read_text(encoding="utf-8").lower()
    for token in ("tinyusb", "tusb", "tud_", "btstack", "cyw43", "hid_host", "gpio_", "spi_"):
        if token in main_text:
            fail(f"G05 app composition crosses a transport/HAL boundary: {token}")


if __name__ == "__main__":
    main()
