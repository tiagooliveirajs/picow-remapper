# REMAPPER-G06 / W10.6 — BLE Mouse passthrough validation

## Scope

G06 introduces the first live Bluetooth input path in the canonical firmware:

`BLE HOGP Mouse -> Report Map parser -> canonical Mouse events -> source ownership aggregator -> fixed USB Mouse`.

The fixed USB Mouse + Keyboard identity from G04 remains present from boot and is never replaced by the remote BLE descriptor.

## Runtime boundary

- Core1 owns BTstack lifecycle through `bt_runtime`.
- `ble_hogp` scans for BLE advertisements containing the HID service UUID, connects one device, requests bonding/security and opens the HIDS client in Report Protocol mode.
- The remote Report Map is parsed inside `ble_hogp`; remote Report IDs and bit offsets never enter domain/remap/USB modules.
- Only canonical Mouse events and connection status cross from Core1 to Core0 through the typed SPSC runtime queue.
- Core0 applies canonical events to the G05 ownership aggregator and forwards the aggregate Mouse state through the fixed G04 USB Mouse interface.
- Queue overflow is explicit. Core0 releases BLE Mouse ownership and the BLE adapter disconnects/restarts after a failed input publish, preventing persistent stuck-button state.
- Bluetooth connect/disconnect never calls TinyUSB connect/disconnect and never changes USB descriptors.

## Report Map coverage in G06

The host-testable parser supports Mouse Application collections with:

- buttons 1 through 8;
- relative X and Y;
- relative vertical Wheel;
- Consumer-page AC Pan for horizontal wheel;
- multiple Report IDs while ignoring non-Mouse reports such as Keyboard input in the same Report Map;
- signed fields up to 32 bits, converted to canonical `int16_t` relative events.

G07 remains responsible for Logitech Lift HID++ diversion/held-state semantics and remapping. G06 validates normal HOGP passthrough only; the known Lift Forward special behavior is not a G06 acceptance blocker.

## Automated acceptance

The G06 host test must pass these numbered scenarios:

1. Parse a Report Map containing Keyboard Report ID 1 and Mouse Report ID 2.
2. Ignore the Keyboard report without emitting canonical Mouse events.
3. Decode Left + Forward button transitions from the Mouse report.
4. Decode signed relative X/Y movement.
5. Decode vertical Wheel and horizontal AC Pan.
6. Do not duplicate held-button press events on repeated Mouse reports.
7. Emit button releases when the corresponding bits clear.
8. Reject a truncated Mouse report rather than reading beyond its payload.
9. Reject a descriptor that contains no usable Mouse fields.
10. Preserve/decode canonical Mouse events through the typed cross-core runtime queue.
11. Detect queue-full overflow explicitly.
12. Preserve pending relative deltas until successfully consumed.
13. Consume large relative motion/wheel in USB-sized chunks without truncation.
14. Release only BLE Mouse persistent ownership on disconnect/teardown.
15. Preserve the fixed G04 USB Mouse report shape.
16. Pass all G01–G05 regression and architecture tests.

## Physical acceptance — LCD/HAT + normal host UI only

No serial terminal is required or accepted for gate evidence.

1. Flash the G06 UF2 and power-cycle. The first visible page must still be `LEARN THE KEYS` with the G04-approved layout.
2. Before pairing any Bluetooth device, the host OS must still expose the fixed Mouse HID + Keyboard HID functions.
3. Put one BLE HOGP mouse into pairing mode near the Pico 2 W. The firmware must discover, pair and connect it without host-side remapper software.
4. Move the BLE mouse in both axes. The host pointer must follow X/Y movement through the Pico USB connection.
5. Verify Left, Right and Middle press/hold/release. A held Left button must support an ordinary drag and release without sticking.
6. Verify vertical wheel. If the mouse provides horizontal wheel/pan, verify it also passes through.
7. If the mouse emits standard Back/Forward HOGP button bits, verify them. For Logitech Lift, Forward's vendor-specific delayed/held semantics are intentionally deferred to G07 and do not fail G06.
8. While holding a Mouse button, force a BLE disconnect (for example, power the mouse off). After disconnect, the USB host must not remain in a stuck pressed/drag state.
9. Turn the mouse back on / put it back into advertising mode. G06 should scan and reconnect during the same firmware boot. Persistence across a Pico reboot is not required until G11.
10. During BLE connect, disconnect and reconnect, the host USB Mouse + Keyboard identity must remain continuously fixed; there must be no USB re-enumeration caused by Bluetooth state.
11. Lock the LCD with Key Y while the BLE mouse is connected. Mouse forwarding must continue while the display is locked. Unlock with one consumed HAT interaction as before.
12. Navigate/render several LCD pages while moving the mouse. UI activity must not change mapping semantics or leave a button stuck.

## Gate boundary

G06 is complete only when automated CI is green and the numbered physical scenarios above are reported as passed on the target Pico 2 W. Do not merge automatically.
