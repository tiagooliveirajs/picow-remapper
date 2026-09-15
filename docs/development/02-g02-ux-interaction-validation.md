# REMAPPER-G02 UX interaction validation

Gate: `REMAPPER-G02` / `W10.2`

## Objective

Implement the hardware-neutral UX/navigation and interaction engine defined by the canonical UX contracts while keeping rendering, HAT GPIO, Bluetooth, USB HID, persistence and remap execution outside this gate.

## Implemented

- `domain` product types used by UX commands: Mouse, Keyboard, Composite and the four mouse profile kinds.
- Application command values emitted by UX without executing transport/profile/storage behavior.
- Compiled `ux_model` library with canonical screen responsibilities and static option metadata.
- Compiled `interaction` library with press/release processing, navigation history, selection, lock/unlock and Help ownership.
- Dynamic option projection tokens for Saved Devices and per-button Custom Remap targets without introducing registry/profile storage dependencies.
- Host C tests for the interaction contract.
- Pico 2 W cross-build of the same UX/interaction libraries.

## Interaction invariants

### Action on release

A press only changes pressed/visual state. Navigation, selection changes, apply, cancel/back, pairing requests, lock and other actions execute only after the matching release event.

A release without a preceding matching press is ignored. This keeps the UI state machine deterministic across debounce/input adapters.

### `LEARN THE KEYS`

The boot screen remains `LEARN THE KEYS`. All controls are demonstration-only except `KEY Y`; `KEY Y` release enters locked state. Joy Left, Joy Right, Joy Press, Joy Up/Down, Key A, Key B and Key X do not leave or act on the screen.

### Locked state

The first complete physical control interaction after lock is consumed only for unlock. Unlock returns to HOME and does not execute that control's normal HOME action.

### Help ownership

Screens explicitly marked as Help-capable enter the generic Help interaction context on Key X release. While Help owns the context, any matched key/control release returns to the previous screen. Key Y therefore returns from Help and does not lock.

### Selection and activation

Entering a static option screen initializes selection to the first option. Joy Up/Down move selection only on release and clamp at list boundaries. Joy Right/Joy Press activate the selected option only on release.

When an actionable key hint (A/B/X/Y) is held, the interaction snapshot stops emphasizing the prior main-body selection. Joy Right/Joy Press preserve selected-option emphasis while held, matching the activation rule in the UX contract.

## Command boundary

The interaction engine emits domain commands but does not implement them. Current G02 command kinds cover:

- start pairing for Mouse, Keyboard or Composite;
- apply one of the documented mouse profiles;
- commit the Custom Remap draft;
- apply one per-source custom mapping choice to the in-memory draft;
- select a Saved Devices projection item;
- remove the selected saved device.

Bluetooth discovery/authentication, profile mutation, persistence and actual removal are later-gate responsibilities.

## Host acceptance

`tests/test_ux_interaction.c` validates:

- boot into `LEARN THE KEYS`;
- no functional action on press;
- action on matching release;
- Key Y lock semantics;
- unlock consumes the triggering interaction and returns HOME;
- normal Home/Mouse/Other navigation;
- Key B one-screen back and Joy Left return HOME;
- Help ownership including Key Y exception;
- pairing command emission on navigation release;
- profile apply on Key A release;
- Custom Remap per-button draft apply and separate final commit;
- Saved Devices dynamic projection selection and remove flow;
- option boundary clamping;
- unmatched releases ignored;
- canonical `FORWARD` spelling in model metadata.

`tests/test_architecture.py` additionally requires `ux_model` and `interaction` to be compiled libraries with only their documented dependency edges.

## Pico build acceptance

The Pico CI job cross-builds the same libraries for `PICO_BOARD=pico2_w`, verifies a non-empty `picow_remapper.uf2`, and publishes the G02 UF2 artifact. The boot executable still does not initialize HAT GPIO, ST7789, Bluetooth or USB HID.

## Physical validation policy

No terminal test is required for G02. Physical interaction verification is intentionally deferred until the renderer/HAT gate provides the LCD-driven test surface. The G02 interaction scenarios above must then be exercised through the display and HAT controls, not through a serial terminal.
