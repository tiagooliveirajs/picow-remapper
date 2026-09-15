# Visual contract

## Grid

Every screen is specified against a fixed text grid:

- 9 rows;
- 21 characters maximum per row;
- title is row 1 in the human page files / first rendered text row;
- remaining main content starts on the next row;
- hints occupy the final rows.

Exact screen layouts must be reviewable in a monospaced editor before implementation.

## Backgrounds

`LEARN THE KEYS` uses the existing very dark magenta background across the entire display.

Every other screen has two visual regions:

- main region: black background;
- hint region: the same very dark magenta used by `LEARN THE KEYS`.

There is always one text-empty row between main content and hints. The horizontal background boundary crosses the vertical midpoint of that empty row.

## Optical vertical centering

The 9x21 semantic grid remains unchanged, but standard two-region screens apply a small optical vertical offset inside each background region:

- the title remains at its original rendered Y position;
- non-title text in the black main region moves downward by 25% of the inter-line pixel gap;
- hint text in the dark-magenta region moves upward by the same amount;
- the empty separator row and the black/magenta background boundary do not move;
- `LEARN THE KEYS`, which uses a single full-screen background, keeps the original row positions.

With the canonical renderer geometry (`LINE_ADVANCE=27`, `GLYPH_HEIGHT=14`), the inter-line gap is 13 px and the integer optical offset is 3 px. This is a pixel-rendering refinement only; screen specifications remain 9 rows by 21 columns.

## Text color semantics

Color expresses meaning and interaction state, not arbitrary per-screen styling.

- **normal magenta**: page title on the first line;
- **yellow**: static main-body text without button action;
- **light gray**: actionable text and hint text in resting state;
- **white**: currently selected option or currently pressed action/hint;
- **cyan**: successful/current/applied state in main content only.

All option labels are indented by one leading space and never use a `>` selection marker.

## Selection/current precedence

An item can be both the current applied setting and currently selected. Visual precedence is:

1. pressed -> white;
2. selected -> white;
3. current/applied/success -> cyan;
4. actionable -> light gray;
5. static -> yellow.

When selection moves away from a current setting, it returns to cyan rather than light gray.

`EDIT CUSTOM REMAP` main-body mapping text is cyan when Custom is the active profile and light gray when another profile is active.

## Successful persistent state

Whenever an operation successfully establishes a persistent/current state — applying a profile, committing custom remap, saving a pairing or equivalent — the relevant main-body confirmation/state text becomes cyan.

Cyan is never a hint-region color.

## Device Detail / Remove Device

For a saved device that is currently active in its type slot, the device name and values after `TYPE:`, `STATUS:` and `PROFILE:` are cyan. For a saved but inactive device, those values are yellow.

This comparison is by `DeviceId` against the active slot, not by display name.

## Renderer implication

The renderer must support semantic color spans/cells within one line. A line-level color API is insufficient because labels and values can have different semantic roles.
