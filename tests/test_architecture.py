#!/usr/bin/env python3
import re
import sys
from pathlib import Path

root = Path(sys.argv[1]).resolve() if len(sys.argv) > 1 else Path(__file__).resolve().parents[1]

module_roots = [root / "src" / "domain", root / "src" / "hid_aggregator"]
forbidden_include_fragments = (
    "btstack",
    "tinyusb",
    "tusb.h",
    "pico/",
    "hardware/",
    "cyw43",
)
remote_layout_identifiers = (
    "report_id",
    "report_descriptor",
    "report_map",
)

errors = []

for module_root in module_roots:
    for path in sorted(module_root.rglob("*")):
        if path.suffix not in {".c", ".h"}:
            continue
        text = path.read_text(encoding="utf-8")
        relative = path.relative_to(root)

        for include in re.findall(r'^\s*#\s*include\s*[<"]([^>"]+)[>"]', text, re.MULTILINE):
            lowered = include.lower()
            for forbidden in forbidden_include_fragments:
                if forbidden in lowered:
                    errors.append(f"{relative}: forbidden platform include {include!r}")

        if path.suffix == ".h":
            for identifier in remote_layout_identifiers:
                if re.search(rf"\b{re.escape(identifier)}\b", text):
                    errors.append(
                        f"{relative}: remote report-layout identifier {identifier!r} leaked into public API"
                    )

        if re.search(r'^\s*#\s*include\s*["<][^">]+\.c[">]', text, re.MULTILINE):
            errors.append(f"{relative}: textual .c inclusion is forbidden")

cmake = (root / "CMakeLists.txt").read_text(encoding="utf-8")
required_cmake_patterns = {
    "domain target": r"add_library\s*\(\s*domain\s+INTERFACE\s*\)",
    "hid_aggregator target": r"add_library\s*\(\s*hid_aggregator\s+STATIC\b",
    "aggregator depends on domain": r"target_link_libraries\s*\(\s*hid_aggregator\s+PUBLIC\s+domain\s*\)",
}
for label, pattern in required_cmake_patterns.items():
    if re.search(pattern, cmake, re.IGNORECASE | re.MULTILINE) is None:
        errors.append(f"CMakeLists.txt: missing {label}")

if errors:
    print("Architecture boundary failures:", file=sys.stderr)
    for error in errors:
        print(f" - {error}", file=sys.stderr)
    sys.exit(1)

print("G05 architecture boundaries passed")
