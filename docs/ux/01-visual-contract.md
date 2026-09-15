# Visual contract

## Grid

Every screen is specified against a fixed text grid:

- 9 rows;
- 21 characters maximum per row;
- title is row 1 in the human page files / first rendered text row;
- remaining main content starts on the next row;
- hints occupy the final rows.

Exact screen layouts must be reviewable in a monospaced editor before implementation.

The 9x21 model is semantic. Pixel Y positions may be redistributed inside the 240 px panel as defined below; row identity, text contents, selection semantics and maximum 21-character width remain unchanged.

## Backgrounds

`LEARN THE KEYS` uses the existing very dark magenta background across the entire display.

Every other screen has two visual regions:

- main region: black background;
- hint region: the same very dark magenta used by `LEARN THE KEYS`.

There is always one semantic text-empty separator row between main content and hints. The pixel boundary between black and dark magenta is layout-dependent and is defined by the number of hint lines rather than by the midpoint of that semantic separator row.

## Vertical pixel distribution

The canonical glyph is 14 px high. The panel is 240 px high and the title starts at Y=8.

### Learn The Keys

`LEARN THE KEYS` is a single full-magenta region and uses all nine semantic rows with this vertical distribution:

- top of panel to title: 8 px;
- title to `UP/DOWN: SELECT`: 17 px;
- every following inter-line gap through `RELEASE RUNS ACTION`: 11 px;
- bottom of `RELEASE RUNS ACTION` to bottom of panel: 12 px.

Rendered text Y positions are therefore: `8, 39, 64, 89, 114, 139, 164, 189, 214`.

### Standard two-region screens

The title remains at Y=8. Main-body slots and hint slots use 12 px inter-line gaps. The empty semantic separator row remains empty even though the physical background boundary is no longer tied to its midpoint.

For every standard layout:

- title to first body line: 17 px;
- body-line to body-line: 12 px;
- last available body slot to black/magenta boundary: 20 px;
- boundary to first hint line: 11 px;
- hint-line to hint-line: 12 px;
- last hint line to bottom of panel: 12 px.

The three supported layouts are:

| Body slots | Hint lines | Black region | Magenta region | Boundary Y |
| ---: | ---: | ---: | ---: | ---: |
| 6 | 1 | 203 px | 37 px | 203 |
| 5 | 2 | 177 px | 63 px | 177 |
| 4 | 3 | 151 px | 89 px | 151 |

Body-slot Y positions are `39, 65, 91, 117, 143, 169` as applicable. Hint-slot Y positions are anchored from the bottom: row 6 = 162, row 7 = 188, row 8 = 214 as applicable.

Missing body text does not collapse the geometry. Empty body slots still reserve the same vertical space so screens with the same number of hint lines share the same background boundary and optical layout.

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
