from __future__ import annotations

import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
MODULE_FILE = ROOT / "cmake" / "RemapperModules.cmake"
ROOT_CMAKE = ROOT / "CMakeLists.txt"
SRC = ROOT / "src"

EXPECTED_MODULES = {
    "domain",
    "ux_model",
    "interaction",
    "renderer",
    "hat",
    "device_registry",
    "profiles",
    "remap",
    "hid_aggregator",
    "usb_hid",
    "bt_runtime",
    "ble_hogp",
    "classic_hid",
    "logitech_hidpp",
    "connection_coordinator",
    "storage_pico",
    "app",
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
    "tud_",
    "tusb_",
    "hid_host_",
    "btstack_",
    "hci_",
    "sm_",
    "cyw43_",
    "gpio_",
    "spi_",
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

    declared = set(
        re.findall(r"remapper_add_module\(\s*([a-z][a-z0-9_]*)\b", module_text)
    )
    if declared != EXPECTED_MODULES:
        fail(f"canonical CMake module set mismatch: {sorted(declared)}")

    for target, expected in EXPECTED_DEPS.items():
        actual = parse_deps(module_text, target)
        if actual != expected:
            fail(f"{target} dependencies {sorted(actual)} != {sorted(expected)}")

    for target in {"domain", "renderer", "hat", "usb_hid", "bt_runtime", "storage_pico"}:
        if parse_deps(module_text, target):
            fail(f"{target} has a forbidden canonical dependency")

    for target in ("ux_model", "interaction"):
        if not re.search(rf"remapper_add_module\(\s*{target}\s+STATIC\b", module_text):
            fail(f"{target} is not a compiled library in G02")

    if 'set(PICO_BOARD "pico2_w"' not in root_cmake:
        fail("canonical Pico board is not fixed to pico2_w")
    if "pico_add_extra_outputs(picow_remapper)" not in root_cmake:
        fail("Pico build does not generate UF2/extra outputs")
    if 'REMAPPER_GATE_NAME="REMAPPER-G02"' not in root_cmake:
        fail("firmware gate marker is not REMAPPER-G02")

    source_files = [
        path
        for path in SRC.rglob("*")
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

    main_text = (ROOT / "src" / "app" / "main.c").read_text(encoding="utf-8").lower()
    for token in ("tinyusb", "tusb", "btstack", "cyw43", "st7789", "hid_host", "gpio_", "spi_"):
        if token in main_text:
            fail(f"G02 boot scaffold contains later-gate behavior token: {token}")


if __name__ == "__main__":
    main()
