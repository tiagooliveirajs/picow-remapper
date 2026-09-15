# REMAPPER-G04 validation — fixed USB Mouse + Keyboard identity

## Objective

Validate a firmware-owned, fixed USB HID identity with separate Mouse and Keyboard interfaces from boot, independent from future Bluetooth topology. This gate also carries the requested incremental optical-centering refinement from G03.

## Automated acceptance

CI must prove:

1. `usb_hid` is a real canonical module and is the only product module allowed to include TinyUSB device APIs.
2. TinyUSB is configured for exactly two HID interfaces: Mouse and Keyboard.
3. USB descriptors are compile-time fixed and contain no Bluetooth/runtime dependency.
4. No `tud_disconnect()` / `tud_connect()` re-enumeration path exists.
5. A standard eight-byte keyboard report can be constructed without any Bluetooth peer or keyboard transport.
6. A standard five-byte mouse report can be constructed independently from Bluetooth.
7. Host tests retain all G02/G03 UX behavior.
8. Standard-screen optical centering uses a 3 px offset: body down, hints up, title/boundary unchanged.
9. Pico 2 W cross-build produces a non-empty UF2.

## Physical scenarios

Use the LCD/HAT and normal graphical host-OS UI only. No serial terminal is required.

1. **Boot regression** — flash G04 and power-cycle. The first visible LCD page is still `LEARN THE KEYS`.
2. **Fixed USB functions** — with no Bluetooth peer connected, inspect the host's graphical device/input settings and confirm the PicoW Remapper exposes both a mouse HID function and a keyboard HID function.
3. **Stable reconnect identity** — unplug and reconnect the Pico 2 W. Confirm the same Mouse + Keyboard functions return; no alternate descriptor/device personality appears.
4. **Keyboard without peer** — while no physical Bluetooth keyboard is paired or connected, confirm the host still lists the keyboard HID function. The transport-independent synthetic keyboard report constructor is covered by CI.
5. **Mouse without peer** — while no Bluetooth mouse is connected, confirm the host still lists the mouse HID function.
6. **Main-region optical centering** — navigate from Learn The Keys to HOME. Confirm the magenta title did not move while the black-region body/options appear slightly lower and more vertically centered than G03.
7. **Hint-region optical centering** — on HOME and at least one other standard page, confirm the hint text appears slightly higher inside the magenta region and no longer visually crowds its upper/lower edge.
8. **Separator invariant** — confirm the empty separator row and black/magenta boundary remain visually unchanged; text must not cross the region boundary.
9. **Interaction regression** — hold then release Joy Down/Right/Press and confirm selection/action still occurs only on release.
10. **Lock regression** — lock with Key Y release, then unlock with one complete interaction; confirm the first interaction is consumed and HOME returns normally.

## Bluetooth/re-enumeration boundary

G04 contains no Bluetooth transport yet, so a live Bluetooth connect/disconnect cannot be physically exercised in this gate. The stronger invariant is encoded now: USB descriptors contain no Bluetooth dependency and the firmware contains no USB disconnect/reconnect path. The first Bluetooth-enabled physical gate must repeat the USB identity check while connecting/disconnecting a peer.

## Gate close

Do not close REMAPPER-G04 until the enumerated physical scenarios that are applicable to this gate pass on the target Pico 2 W and host OS.
