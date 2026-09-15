# Architecture overview

## Architectural goal

The firmware is a product-specific remapper, not an adaptation layer around the original HOGP demo. The old `picow-mouse-remapper` repository is evidence and reference only; this repository must not depend structurally on its source tree.

## Primary flow

```text
BLE Mouse ---------\
                    \
Classic Keyboard ----> Canonical input -> Remap (Mouse slot only)
                    /                         |
BLE Composite ------/                          v
                                      Input Ownership Aggregator
                                                 |
                                                 v
                                      Fixed USB Mouse + Keyboard
```

The UX runs in parallel:

```text
HAT -> Input Driver -> Interaction Engine -> UX State Machine -> Application Commands
```

UX does not sit in the HID forwarding path. The remapper must continue operating while the screen is locked or while the user remains on `LEARN THE KEYS`.

## Governing invariants

1. USB Mouse + Keyboard identity is fixed from boot and does not re-enumerate because a Bluetooth peer connects/disconnects.
2. Remote HID report IDs/layouts never escape the transport/parser layer.
3. Domain/remap code does not depend on BTstack, TinyUSB, Pico SDK or LCD drivers.
4. UI never calls BTstack or TinyUSB directly.
5. Core0 never calls BTstack; Core1 never renders the LCD.
6. A single Bluetooth Runtime owns initialization and operation of BTstack.
7. Press provides feedback; release executes UX action.
8. Disconnect/replacement releases only the ownership of the affected input source, never all keyboard/mouse state globally.
9. Pairing a new peer is transactional and cannot destroy the current saved peer before candidate validation/commit.
10. Documentation is the product contract; implementation changes that alter behavior require documentation change first.

## Runtime cores

Core0 owns application/domain state, UX, HAT, renderer, profiles, remap, ownership aggregation and TinyUSB.

Core1 owns BTstack and its Bluetooth sessions: BLE Mouse HOGP, BLE Composite HOGP, Classic Keyboard HID and Logitech HID++ integration.

Communication crosses the core boundary only through typed command/event queues or equivalent explicit message channels.
