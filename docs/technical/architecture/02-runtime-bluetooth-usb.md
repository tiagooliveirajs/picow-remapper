# Runtime, Bluetooth and USB

## Bluetooth Runtime ownership

Exactly one module owns BTstack initialization and run-loop lifecycle. Transport adapters register/use sessions through that runtime; they do not independently initialize L2CAP/SM/HCI.

Planned sessions are:

- BLE HOGP Mouse;
- BLE HOGP Composite;
- Bluetooth Classic HID Keyboard.

The maximum product topology therefore requires two simultaneous BLE HOGP connections plus one Classic HID connection. This topology was **not** proven by the old project and is an explicit implementation risk gate.

## Canonical HID

Each transport/parser emits transport-independent events such as `CanonicalMouseEvent` and `CanonicalKeyboardEvent` with source identity. Remote Report IDs and descriptor layouts remain inside parser/transport modules.

Mouse flow:

`BLE HOGP -> HID parser / vendor quirk -> canonical mouse -> remap -> ownership aggregator`.

Keyboard flow:

`Classic HID -> keyboard parser -> canonical keyboard -> ownership aggregator`.

Composite flow:

`BLE HOGP -> Report Map parser -> canonical mouse + canonical keyboard -> ownership aggregator`.

## Logitech backend

Known Logitech behavior is resolved automatically from device identity/capabilities. The Lift Forward held-state behavior proven in the previous POC is represented as an implementation backend using HID++ diversion/down/hold/up semantics. HID++ is not a user-facing option in v1.

## Input Ownership Aggregator

The aggregator owns final USB button/key state by source. Multiple sources may own the same logical button/key concurrently. A release from one source must not release a state still owned by another source.

This applies to:

- Mouse slot and Composite both holding Mouse Left;
- Keyboard slot and Composite both holding the same key/modifier;
- synthetic keyboard output from mouse remap (for example Escape) concurrent with a physical keyboard key.

Disconnect removes only the disconnected source's ownership.

## USB

TinyUSB exposes a firmware-owned fixed composite USB device with Mouse + Keyboard from boot. Bluetooth topology changes do not alter descriptors or force host re-enumeration.

Mouse-to-keyboard remaps therefore work even when no physical keyboard is connected.

## Concurrency boundary

Core0 and Core1 communicate through typed commands/events. Bluetooth callbacks are converted to messages before entering product/domain logic. No LCD rendering or direct UI callback is allowed from the Bluetooth core.
