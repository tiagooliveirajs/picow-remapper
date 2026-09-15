# REMAPPER-G05 validation — canonical HID and source ownership

## Objective

Introduce the transport-independent canonical HID boundary and source-aware ownership/refcount aggregation required before Bluetooth adapters are added in G06+.

The G04 USB identity, LCD layout and interaction behavior must remain unchanged.

## Canonical HID contract

- A source identity is `{kind, instance}` and distinguishes Mouse, Keyboard, Composite and Synthetic Remap output.
- Mouse canonical events are button, relative movement and wheel/pan.
- Keyboard canonical events are logical key and modifier press/release.
- Logical keyboard keys use the USB HID usage namespace as stable identity only; remote descriptors/report IDs never enter the domain API.
- Up to 16 simultaneously tracked ownership sources are supported.

## Ownership contract

- Persistent Mouse buttons, Keyboard keys and modifiers are owned per source.
- Aggregate output remains pressed while at least one source owns the logical control.
- Duplicate press/release from the same source is idempotent.
- Releasing/disconnecting a source removes only that source's persistent ownership.
- Synthetic remap Keyboard ownership can overlap physical Keyboard ownership without causing an early release.
- Relative Mouse movement and wheel input are transient: they accumulate across sources and are consumed separately from persistent ownership.
- Relative-only events do not require persistent source ownership slots.

## Automated scenarios

1. **Source validation** — valid canonical source kinds are accepted; invalid/unknown kinds are rejected.
2. **Shared Mouse ownership** — Mouse and Composite hold Left simultaneously; releasing either one leaves Left held until both release.
3. **Mouse idempotence** — duplicate press/release from one source does not double-count or underflow refs.
4. **Shared Keyboard key** — Keyboard and Composite hold the same key; source-selective release preserves the remaining owner.
5. **Shared modifier** — Keyboard and Composite hold Left Shift; source-selective release preserves the remaining owner.
6. **Disconnect isolation** — releasing one Mouse source clears only its unique buttons and preserves buttons shared with another source.
7. **Synthetic coexistence** — Synthetic Remap and physical Keyboard can hold Escape simultaneously; removing synthetic ownership does not release the physical key.
8. **Relative aggregation** — movement and wheel deltas from multiple sources sum correctly, survive snapshot, and are cleared only by take-output.
9. **Source capacity** — all 16 persistent ownership slots can be used; a 17th persistent source is rejected while full.
10. **Slot reuse** — after one source is released, the freed slot can be reused by a new source.
11. **Architecture boundary** — canonical HID and aggregator contain no BTstack, TinyUSB, Pico SDK/CYW43, GPIO/SPI, HID-host or remote report-layout dependencies.
12. **Regression build** — all G01–G04 host tests remain green and the Pico 2 W G05 firmware still builds to a non-empty UF2.

## Physical validation

None required for G05. The planner evidence boundary for REMAPPER-G05 / W10.5 is host automated testing. The LCD/HAT and USB behavior are regression-built but no new physical behavior is introduced in this gate.

## Gate close

G05 can be accepted when all automated scenarios above pass in both push and pull-request CI on the final G05 head. Do not merge automatically.
