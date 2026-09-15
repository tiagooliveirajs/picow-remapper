# Navigation and screen model

This page defines screen responsibilities. Exact 9x21 page text is a UX artifact and must obey the interaction and visual contracts.

## Root

`LEARN THE KEYS` is the boot screen. It does not block background reconnect. Unlocking from the locked state enters HOME.

HOME provides the normal navigation root:

- STATUS;
- MOUSE OPTIONS;
- OTHER OPTIONS;
- LEARN THE KEYS.

## Mouse Options

`MOUSE OPTIONS` owns mouse-specific configuration:

- Pair Mouse;
- Passthrough;
- Default Remap;
- Escape Remap;
- Custom Remap.

Pairing success returns a persistent success state. The four profile choices are per saved Mouse.

`EDIT CUSTOM REMAP` exposes per-source mapping pages for Left, Right, Middle, Forward and Backward. Each mapping page chooses a target, uses `KEY A: APPLY THIS`, updates only the custom draft, and returns to `EDIT CUSTOM REMAP`. The final custom apply is separate.

## Other Options

`OTHER OPTIONS` contains device-management paths not specific to mouse remapping:

- Pair Keyboard;
- Pair Composite;
- Saved Devices.

Pairing screens may define Help where protocol terminology needs explanation.

## Saved Devices

Saved Devices is paginated because multiple peers of every type may be stored. Pages are projections of one registry, not separate storage buckets. The page indicator is informational and follows the visual contract.

Selecting a saved record opens one `DEVICE DETAILS` view for that record. Mouse details include profile. Keyboard/Composite details do not expose mouse profile in v1.

`REMOVE DEVICE` is one confirmation action. The UI explains the user-visible consequence; internal bond/link-key/profile cleanup is not exposed as separate erase choices.

## Status

STATUS is read-only inspection. It must not begin discovery simply by being opened. It can summarize active slots, mouse profile/backend and storage health. Help may explain abbreviations.

## Pairing vs reconnect

Opening a pairing flow explicitly requests discovery/authentication for a new/repaired peer. Automatic boot reconnect is independent from these screens and may succeed while any ordinary screen — especially `LEARN THE KEYS` — is visible.

## Help

Help is contextual and optional. When present it owns the interaction context and `ANY KEY: BACK` applies.
