# Interaction contract

## Fundamental rule: action on release

No navigation, apply, cancel, lock, retry, removal or other function is executed on button press. Press changes visual state. The corresponding action executes on release.

For an actionable word or hint:

- press: its text becomes white;
- release: the action executes;
- while a hint is pressed, the previously selected main-body option stops rendering as selected and returns to its non-selected semantic color.

For an already selected option activated with Joy Right or Joy Press, the option remains white while the control is held and advances on release.

## Normal-screen global policy

Unless a screen declares one of the exceptions below:

- **Joy Up / Down**: move selection among options.
- **Joy Right / Joy Press**: advance/access the selected option when the current flow has a next screen.
- **Joy Left**: return to HOME.
- **Key A**: primary `apply` / `ok` action for that context.
- **Key B**: cancel or go back exactly one screen. It has no function on HOME.
- **Key Y**: lock the screen from any normal screen.
- **Key X**: opens Help only on screens that actually define a Help screen. Help is not mandatory when the page itself is sufficiently explanatory.

Entering any option screen initializes selection to the first option. The selected option is white.

## `LEARN THE KEYS` exception

`LEARN THE KEYS` is the first screen shown after power-on and is intentionally more didactic than globally consistent.

On this screen every control except Key Y is demonstration-only:

- press changes only the corresponding word/legend from light gray to white;
- release changes it back to light gray;
- Joy Left does **not** go HOME;
- Joy Right and Joy Press do **not** advance;
- Joy Up/Down do **not** select;
- Key A does **not** apply;
- Key B does nothing;
- Key X does not open Help.

Key Y is the sole exception. Its label/arrow receives the same press feedback, and **Key Y release locks the screen**.

This is intentional because the common daily flow is: power on, remain on `LEARN THE KEYS` while saved devices reconnect automatically, use the remapper, then lock the screen/backlight with Key Y.

## Locked state

Lock affects only the HAT/UI presentation. Bluetooth, USB HID, remapping, ownership state and reconnect continue running.

While locked, the first physical control interaction is consumed only for unlock; its normal function does not also execute. Unlock returns to HOME.

## Help exception

On a Help screen **ANY KEY: BACK**, including Key Y. Key Y does not lock while Help owns the interaction context.

## Custom Remap apply semantics

A per-button mapping screen uses **Key A: APPLY THIS**. On release it updates the in-memory custom draft and returns to `EDIT CUSTOM REMAP`.

That does not yet persist the whole custom profile. `EDIT CUSTOM REMAP` owns the explicit final apply of the custom draft.

Key B remains the one-screen back/cancel behavior even when not displayed in the hint region, except on HOME and `LEARN THE KEYS` where it does nothing.

## Hint policy

The hint region displays one function per line, at least one and at most three functions. Not every available global action must be shown. Hidden behavior is allowed when defined by this contract (for example Key B back on ordinary screens).

## Terminology normalization

Canonical product text uses:

- `KEY X`, never `KEY C`;
- `FORWARD`, never `FOREWARD` or `FOREWARED`.
