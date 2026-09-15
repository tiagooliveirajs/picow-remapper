# PicoW Remapper

PicoW Remapper is a standalone Raspberry Pi Pico 2 W HID remapper designed to work without host-side software. It receives Bluetooth HID input, applies product-defined mouse remapping, and exposes a fixed USB HID Mouse + Keyboard interface to the host.

This repository is documentation-first: product and architecture contracts are defined before firmware implementation. Code is a consequence of those contracts, not the source of product behavior.

## Start here

- [Documentation hub](docs/README.md)
- [Product contract](docs/product/00-product-contract.md)
- [Interaction contract](docs/ux/00-interaction-contract.md)
- [Visual contract](docs/ux/01-visual-contract.md)
- [Navigation and screen model](docs/ux/02-navigation-and-screens.md)
- [Architecture overview](docs/technical/architecture/00-overview.md)
- [POC evidence](docs/reference/00-poc-evidence.md)
- [Capability status](docs/reference/01-capability-status.md)

## Current status

The architecture and UX baseline are specified. Firmware implementation gates are planned externally in `tiagooliveirajs/infra-planner`; none of those gates are considered executed merely because this documentation exists.
