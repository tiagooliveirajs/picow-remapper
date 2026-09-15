# Capability status

Status language distinguishes product specification from evidence.

| Capability | Status | Basis |
|---|---|---|
| 9x21 Waveshare HAT text UI geometry | POC-proven | previous physical display validation |
| HAT buttons/joystick and screen lock concept | POC-proven | previous physical validation |
| Fixed USB Mouse + Keyboard from boot | POC-proven | previous bridge work |
| BLE HOGP mouse passthrough | POC-proven | previous mouse work |
| Logitech Lift Forward HID++ held-state correction | POC-proven | previous Lift POC/remap validation |
| Per-mouse profiles / Default and Custom remap concept | POC-proven in prior architecture | must be reimplemented under new contracts |
| Goldentec BKB-3G Classic HID host path | POC-proven at transport/integration level | previous Classic POC/integration |
| Many saved peers per type | Specified | new product model; persistence implementation pending |
| One active peer per Mouse/Keyboard/Composite slot | Specified | new runtime model |
| Automatic boot reconnect while remaining on Learn The Keys | Specified | implementation pending |
| BLE Composite Mouse+Keyboard | Specified | physical target/evidence pending |
| Mouse + Composite simultaneous HOGP | Unproven | dedicated concurrency gate required |
| Mouse + Keyboard + Composite simultaneous topology | Unproven | dedicated 2 BLE + 1 Classic gate required |
| Input ownership/refcount across all sources | Specified | implementation/test pending |
| Documentation-generated/validated screen model | Specified direction | implementation pending |

No item moves from `Specified`/`Unproven` to proven solely because code compiles; required runtime/physical evidence must be recorded by the appropriate implementation gate.
