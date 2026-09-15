# REMAPPER-G01 bootstrap validation

Gate: `REMAPPER-G01` / `W10.1`

## Objective

Establish the minimal canonical implementation skeleton without adding USB, Bluetooth, renderer, HAT, persistence, remap or product interaction behavior that belongs to later gates.

## Implemented

- Host-testable CMake project.
- Pico 2 W (`pico2_w`) cross-build scaffold.
- Canonical CMake targets for every library named by `03-module-boundaries.md`.
- Dependency direction encoded between canonical targets.
- Minimal Pico boot executable with no USB/Bluetooth/LCD behavior.
- Host compile smoke test.
- Automated architecture boundary test.
- CI jobs for host validation and Pico 2 W UF2 generation.

## Acceptance mapping

### CMake project builds for host tests and Pico target scaffolding

Validated by the `host-architecture` and `pico2-w-scaffold` CI jobs. The Pico job must generate a non-empty `picow_remapper.uf2`.

### Libraries follow documented ownership boundaries

`cmake/RemapperModules.cmake` declares the canonical targets and only the dependency edges needed at scaffold time. Later-gate hardware dependencies are intentionally absent.

`tests/test_architecture.py` rejects:

- missing/extra canonical module targets;
- unexpected scaffold dependency edges;
- textual inclusion of `.c` files;
- macro interception of TinyUSB/BTstack/CYW43/GPIO/SPI-style APIs;
- structural source dependency on the legacy `picow-mouse-remapper` repository;
- later-gate USB/Bluetooth/LCD behavior in the G01 boot executable.

### CI runs host tests and architecture/spec checks

The host job configures/builds the project and executes CTest. The Pico job independently cross-builds for Pico 2 W and verifies the UF2 artifact.

## Physical validation policy

G01 requires build/CI/architecture evidence only. No terminal or physical interaction test is required for this gate. Physical HAT/display validation belongs to its dedicated canonical gate.
