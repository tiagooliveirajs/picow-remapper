# REMAPPER-G04 validation — fixed USB Mouse + Keyboard identity

## Objective

Validate a firmware-owned, fixed USB HID identity with separate Mouse and Keyboard interfaces from boot, independent from future Bluetooth topology. This gate also carries the approved vertical layout redistribution requested after the first G04 physical pass.

## Automated acceptance

CI must prove:

1. `usb_hid` is a real canonical module and is the only product module allowed to include TinyUSB device APIs.
2. TinyUSB is configured for exactly two HID interfaces: Mouse and Keyboard.
3. USB descriptors are compile-time fixed and contain no Bluetooth/runtime dependency.
4. No `tud_disconnect()` / `tud_connect()` re-enumeration path exists.
5. A standard eight-byte keyboard report can be constructed without any Bluetooth peer or keyboard transport.
6. A standard five-byte mouse report can be constructed independently from Bluetooth.
7. Host tests retain all G02/G03 interaction behavior.
8. `LEARN THE KEYS` renders at Y positions `8,39,64,89,114,139,164,189,214`, yielding 17 px after the title, 11 px between the remaining lines and 12 px below the last line.
9. Standard 6+1, 5+2 and 4+3 layouts use boundaries Y=203, Y=177 and Y=151 respectively, with 17 px title/body gap, 12 px internal body/hint gaps, 20 px body-to-boundary gap, 11 px boundary-to-first-hint gap and 12 px bottom margin.
10. Empty body slots do not collapse or move the background boundary.
11. Pico 2 W cross-build produces a non-empty UF2.

## Physical scenarios

Use the LCD/HAT and normal graphical host-OS UI only. No serial terminal is required.

1. **Boot regression** — flash G04 and power-cycle. The first visible LCD page is still `LEARN THE KEYS`.
2. **Learn title spacing** — confirm `LEARN THE KEYS` remains 8 px below the panel top and the gap to `UP/DOWN: SELECT` is visibly larger than before.
3. **Learn compact internal spacing** — confirm all seven gaps from `UP/DOWN: SELECT` through `RELEASE RUNS ACTION` are uniformly tighter, with no overlap or clipping.
4. **Learn bottom margin** — confirm `RELEASE RUNS ACTION` has a clearly larger magenta margin below it and remains fully visible.
5. **Fixed USB functions** — with no Bluetooth peer connected, inspect the host's graphical device/input settings and confirm the PicoW Remapper exposes both a mouse HID function and a keyboard HID function.
6. **Stable reconnect identity** — unplug and reconnect the Pico 2 W. Confirm the same Mouse + Keyboard functions return; no alternate descriptor/device personality appears.
7. **Keyboard without peer** — while no physical Bluetooth keyboard is paired or connected, confirm the host still lists the keyboard HID function. The transport-independent synthetic keyboard report constructor is covered by CI.
8. **Mouse without peer** — while no Bluetooth mouse is connected, confirm the host still lists the mouse HID function.
9. **6 body + 1 hint geometry** — open a screen that uses one hint line (for example HELP). Confirm the black region extends to Y=203, the hint line sits comfortably inside the magenta footer and the footer has a 12 px bottom margin.
10. **5 body + 2 hints geometry** — open HOME or another two-hint screen. Confirm the black/magenta boundary is at Y=177, body text is evenly packed, and the two hint lines are evenly packed with the lower line 12 px above the panel bottom.
11. **4 body + 3 hints geometry** — open STATUS, Pairing, Profile Preview, Map or another three-hint screen. Confirm the boundary is at Y=151, the body retains reserved space even when some body rows are empty, and all three hints remain fully inside the magenta footer.
12. **Boundary replacement** — confirm the old fixed separator-midpoint boundary is gone: the boundary visibly changes with 1, 2 or 3 hint lines while the semantic separator row remains empty.
13. **No text crossing** — on each of the three standard layouts, confirm no glyph touches or crosses the black/magenta boundary.
14. **Interaction regression** — hold then release Joy Down/Right/Press and confirm selection/action still occurs only on release.
15. **Lock regression** — lock with Key Y release, then unlock with one complete interaction; confirm the first interaction is consumed and HOME returns normally.

## Bluetooth/re-enumeration boundary

G04 contains no Bluetooth transport yet, so a live Bluetooth connect/disconnect cannot be physically exercised in this gate. The stronger invariant is encoded now: USB descriptors contain no Bluetooth dependency and the firmware contains no USB disconnect/reconnect path. The first Bluetooth-enabled physical gate must repeat the USB identity check while connecting/disconnecting a peer.

## Gate close

Do not close REMAPPER-G04 until the enumerated physical scenarios that are applicable to this gate pass on the target Pico 2 W and host OS.
