# REMAPPER-G05 validation — Canonical HID and source ownership

This gate implements W10.5 as a host-testable boundary. It does not claim completion of the earlier canonical hardware/USB gates.

## Scope

- `domain` defines transport-independent HID source identity and canonical Mouse/Keyboard events using standard C only.
- `hid_aggregator` owns persistent button/key/modifier state per source and derives final aggregate output through refcounts.
- Relative pointer/wheel motion is accumulated as transient output and consumed independently from held ownership.
- Bluetooth report IDs, Report Maps, descriptor layouts, BTstack, TinyUSB and Pico SDK types are outside both public APIs.

## Automated acceptance

`canonical_hid_ownership` verifies:

1. Mouse and Composite may both hold Mouse Left; releasing either one alone does not release the aggregate button.
2. Keyboard and Composite may both hold the same key/modifier; disconnecting one source preserves the other owner's state.
3. Source teardown releases only controls owned by that source.
4. A synthetic remap keyboard source can hold Escape concurrently with a physical Keyboard source without premature release.
5. Duplicate press/release input is idempotent.
6. Relative motion from multiple sources is combined and consumed without becoming persistent held state.

`architecture_boundaries` verifies:

- no BTstack, TinyUSB, Pico SDK, CYW43 or hardware HAL includes enter `domain` or `hid_aggregator`;
- no textual `.c` inclusion is introduced;
- remote report-layout identifiers do not enter public headers;
- `domain` and `hid_aggregator` remain distinct CMake targets and the dependency direction is `hid_aggregator -> domain`.

## Evidence boundary

REMAPPER-G05 requires commit + automated host test evidence. No physical Pico, terminal, LCD or HAT test is required for this gate.

The earlier experimental work in `picow-mouse-remapper` remains implementation evidence/reference only. Canonical transport integration belongs to later gates (G06/G08/etc.) and must consume these canonical APIs instead of copying remote HID layouts into the USB-facing model.
