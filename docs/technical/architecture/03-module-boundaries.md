# Module boundaries

Each logical component should be a real CMake library/target with a small public header. Source files are never textually included from other `.c` files, and global macro interception must not be used to change another module's runtime behavior.

| Library | Owns | May depend on |
|---|---|---|
| `domain` | IDs, device/profile types, canonical HID, application commands | standard C only |
| `ux_model` | screen/navigation model, semantic cells | domain |
| `interaction` | press/release, selection, lock/help/learn policy | ux_model, domain |
| `renderer` | semantic cells -> ST7789 pixels | display HAL |
| `hat` | GPIO/joystick/buttons -> input events | Pico HAL |
| `device_registry` | saved/preferred/active model | domain, storage interface |
| `profiles` | per-mouse profiles, custom draft/commit | domain, storage interface |
| `remap` | canonical mouse transformations | domain |
| `hid_aggregator` | source ownership/refcounts | domain |
| `usb_hid` | aggregate canonical state -> TinyUSB | TinyUSB adapter |
| `bt_runtime` | BTstack lifecycle/session ownership | BTstack/Pico Bluetooth HAL |
| `ble_hogp` | BLE Mouse/Composite session adapter | bt_runtime, domain |
| `classic_hid` | Classic Keyboard session adapter | bt_runtime, domain |
| `logitech_hidpp` | Logitech vendor quirk backend | BLE session interface, domain |
| `connection_coordinator` | reconnect/pair/switch/remove policy | registry + transport interfaces |
| `storage_pico` | flash/TLV persistence implementation | Pico SDK |
| `app` | composition/orchestration and projections | public interfaces above |

Forbidden dependency examples:

- UI -> BTstack;
- UI -> TinyUSB;
- Remap -> BTstack;
- HID++ -> LCD;
- Classic Keyboard -> renderer;
- Domain -> Pico SDK;
- Bluetooth transport -> product storage implementation.

The intent is that domain, remap, ownership and major UX state-machine behavior can be compiled/tested on a host without Pico hardware.
