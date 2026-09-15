# REMAPPER-G03 / W10.3 validation — Waveshare renderer, Learn The Keys and lock

## Scope

G03 materializes the canonical `renderer` and `hat` modules for the Waveshare Pico-LCD-1.3 / ST7789 surface on Pico 2 W. It binds the already host-tested G02 interaction engine to physical buttons/joystick and to the LCD without introducing USB HID, Bluetooth, profiles, persistence or pairing runtime behavior from later gates.

The retained physical POC mapping is re-expressed behind the new module boundaries:

- ST7789 over SPI1: DC=GPIO8, CS=GPIO9, SCK=GPIO10, MOSI=GPIO11, RST=GPIO12, BL=GPIO13;
- joystick: Up=GPIO2, Press=GPIO3, Left=GPIO16, Down=GPIO18, Right=GPIO20;
- keys: A=GPIO15, B=GPIO17, X=GPIO19, Y=GPIO21;
- all HAT controls are active-low with pull-ups and 20 ms debounce.

## Automated evidence

Host tests must prove the 9x21 geometry, semantic color model, Learn The Keys press-only feedback, release-triggered Key Y lock, consumed unlock to HOME, standard black-main/dark-magenta-hint split with an empty separator row, retained GPIO mapping and architecture boundaries. Pico SDK 2.2.0 CI must cross-build `pico2_w` and produce a non-empty UF2. Automated build evidence is necessary but cannot satisfy the physical G03 gate by itself.

## LCD/HAT physical acceptance — no terminal required

Flash the G03 UF2 by the normal BOOTSEL file-copy flow. All acceptance observations are made only on the LCD/HAT.

1. **Boot surface** — power-cycle the Pico 2 W. The first visible content must be `LEARN THE KEYS`; no test pattern or HOME page may flash before it.
2. **Learn feedback** — hold each joystick direction, joystick press, Key A, Key B and Key X one at a time. Only that control's legend becomes white while held. Releasing it returns the legend to light gray and the screen remains `LEARN THE KEYS`.
3. **Key Y lock timing** — hold Key Y. Its legend becomes white but the display stays on. Release Key Y; only then the backlight turns off.
4. **Consumed unlock** — while locked, press and release any one physical control. The backlight returns and the page is HOME with `STATUS` selected. That same interaction must not also navigate, apply or lock again.
5. **Standard geometry** — on HOME, verify magenta title, black main region, light-gray options with selected `STATUS` white, dark-magenta hint region, and a visibly empty separator row between main content and hints.
6. **Release-triggered navigation** — hold Joy Down on HOME: selection must not move while held. Release: selection moves exactly once. Hold Joy Right or Joy Press: selected text remains white and the page does not advance until release.
7. **Lock from normal page** — from HOME or another normal screen, hold Key Y: only the hint feedback changes while held. Release turns the backlight off. Unlock again with one consumed interaction and verify HOME is restored.

Record pass/fail for all seven items. REMAPPER-G03 remains physically pending until this LCD/HAT evidence is supplied; serial-terminal output is neither required nor accepted as the physical evidence for this gate.
