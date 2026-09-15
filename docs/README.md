# PicoW Remapper documentation

This documentation is the canonical product and architecture baseline for PicoW Remapper. When documentation and implementation disagree, the discrepancy must be resolved explicitly; implementation must not silently redefine the product.

## Product

- [Product contract](product/00-product-contract.md) — purpose, device model, profiles and everyday experience.

## UX

- [Interaction contract](ux/00-interaction-contract.md) — button semantics, press/release rules, lock/help/learn exceptions and selection behavior.
- [Visual contract](ux/01-visual-contract.md) — 9x21 geometry, color semantics, content/hint regions and rendering precedence.
- [Navigation and screen model](ux/02-navigation-and-screens.md) — canonical screen tree and screen responsibilities.

## Technical architecture

- [Architecture overview](technical/architecture/00-overview.md) — runtime shape and governing invariants.
- [Domain and device model](technical/architecture/01-domain-and-devices.md) — saved, preferred and active devices; profiles and persistence.
- [Runtime, Bluetooth and USB](technical/architecture/02-runtime-bluetooth-usb.md) — Core0/Core1 boundary, transports, canonical HID and ownership aggregation.
- [Module boundaries](technical/architecture/03-module-boundaries.md) — library ownership and allowed dependencies.

## Reference

- [POC evidence](reference/00-poc-evidence.md) — knowledge retained from `picow-mouse-remapper` without structural dependency on it.
- [Capability status](reference/01-capability-status.md) — specified, POC-proven and still-unproven capabilities.

## Development process

- [Documentation-first development](development/00-documentation-first-process.md) — how future changes move from product definition to implementation.
