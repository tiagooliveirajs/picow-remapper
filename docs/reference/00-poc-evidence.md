# POC evidence retained from `picow-mouse-remapper`

The previous repository is a knowledge/evidence source, not a codebase dependency. New implementation may study and selectively re-express proven logic behind the new module contracts.

## Mouse / Logitech Lift evidence

The previous work established that a Raspberry Pi Pico W/2 W class design can receive BLE HOGP mouse reports and forward a fixed USB HID interface. For the Logitech Lift, the POC identified Logitech HID++ `REPROG_CONTROLS_V4` behavior and proved diversion of the Forward control with down/hold/up semantics sufficient to correct the delayed/held-state drag behavior. The product decision is to keep this as an automatic quirk backend rather than a UX setting.

Known source-specific behavior to preserve conceptually:

- Lift Forward can use HID++ held-state correction when the active profile remaps that source;
- Lift Backward remains Standard HID in the default policy;
- generic mice can still use Standard HID remapping when no vendor backend is required.

## Canonical HID evidence

The prior project demonstrated the architectural value of converting remote report layouts into firmware-owned canonical Mouse/Keyboard output rather than leaking remote Report IDs/descriptors into USB behavior. The new project preserves this principle with a cleaner transport-independent domain model.

## Keyboard / Goldentec BKB-3G evidence

The keyboard investigation established that the Goldentec GT T1 / BKB-3G target is Bluetooth Classic HID rather than the BLE HOGP path originally attempted. The Classic POC/integration established inquiry/pairing/HID-host/report-forwarding knowledge that can be re-expressed behind a `classic_hid` adapter.

## What was not proven

The previous project did not provide accepted evidence for the new maximum topology of two concurrent BLE HOGP peers (Mouse + Composite) plus one Bluetooth Classic HID Keyboard. The previous BTstack configuration was sized for fewer simultaneous clients/connections. This must remain a dedicated implementation/validation gate.

The previous project also accumulated experimental coupling in UI/pairing/transport integration. None of those wrappers, `.c` textual includes or macro interception techniques are architectural precedent for this repository.
