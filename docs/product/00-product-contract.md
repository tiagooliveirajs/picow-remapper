# Product contract

## Purpose

PicoW Remapper is a plug-and-play HID appliance. The host sees a stable USB Mouse + Keyboard from boot. Bluetooth devices may connect, disconnect or reconnect without changing that USB identity.

The most common daily experience is intentionally navigation-free:

1. power the Pico;
2. `LEARN THE KEYS` appears immediately;
3. saved devices reconnect automatically in the background while that screen remains visible;
4. input works as soon as the relevant transport is active;
5. the user may press and release Key Y to lock the screen/backlight for dark environments without stopping Bluetooth, USB or remapping.

The UI configures and inspects an autonomous remapper; it is not required for normal reconnect or runtime forwarding.

## Device classes

The product recognizes three logical device types:

- **Mouse** — mouse-capable device; v1 remapping applies only to the active Mouse slot.
- **Keyboard** — keyboard-only device; initial validated transport target is Bluetooth Classic HID for the Goldentec BKB-3G class of keyboard.
- **Composite** — one BLE HID device whose Report Map exposes both Mouse and Keyboard. It is one physical device, one saved record and one connection, not a synthetic pairing of separately saved mouse and keyboard devices.

## Saved, preferred and active are different concepts

The registry may persist multiple devices of each type. Runtime has at most one active device per type:

- active Mouse: zero or one;
- active Keyboard: zero or one;
- active Composite: zero or one.

Therefore these combinations are supported by the product contract:

- Mouse + Keyboard;
- Mouse + Composite;
- Keyboard + Composite;
- Mouse + Keyboard + Composite.

A **saved device** is a persistent known peer. A **preferred device** is the saved peer the reconnect coordinator tries first for that type. An **active device** currently owns that runtime slot. Replacing an active device never deletes the previous saved record.

## Automatic reconnect

At boot the application loads persistent state and starts reconnect work independently of UX navigation. Preferred peers are attempted first. If a preferred peer is unavailable, another saved peer of the same type may be eligible according to the connection policy without erasing the preferred record.

Pairing is only for adding a new peer or repairing one whose credentials are no longer usable. Normal power-on behavior must not require re-pairing.

## Pairing transaction

Pairing a new peer must not destroy a currently working saved/active peer before the new candidate is validated. The intended transaction is:

`discover -> authenticate -> classify -> persist credentials + record -> commit active/preferred transition`.

Failure before commit leaves the previous device intact.

## Saved Devices

`SAVED DEVICES` is a paginated projection of the persistent registry, not a three-row slot list. It can show many Mouse, Keyboard and Composite records across pages. Selecting one opens `DEVICE DETAILS` for that single record.

Removing a device is one user action. Internally it removes the device record, its Bluetooth credentials, its mouse profile when applicable, associated quirk/configuration data, active-slot ownership if present, and preferred reference if present. The user is not asked to erase these internal parts separately.

## Mouse profiles

Every saved Mouse owns its own persisted profile. A newly paired Mouse starts in `PASSTHROUGH`.

The v1 profile kinds are:

- `PASSTHROUGH`;
- `DEFAULT REMAP`;
- `ESCAPE REMAP`;
- `CUSTOM REMAP`.

Preset mappings are product specification, not user-editable custom profiles. `CUSTOM REMAP` is edited as a draft and committed only through its explicit apply action.

Composite mouse input remains passthrough in v1 unless the product contract is explicitly changed later.

## POC-derived Logitech behavior

Known Logitech quirks are resolved automatically from device identity/capabilities. Logitech HID++ is an implementation backend, not a user-facing configuration concept. The Forward held-state/drag behavior proven with the Lift is applied automatically when the selected mouse profile requires it.
