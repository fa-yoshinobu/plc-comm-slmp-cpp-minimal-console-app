# SLMP C++ Minimal Console App

Interactive console applications for `plc-comm-slmp-cpp-minimal`.

This repository hosts the board-specific console programs that were split out of the minimal library repository so the library can stay focused on the core and high-level API.

## Included Console Targets

- `examples/atom_matrix_serial_console`
- `examples/w6300_evb_pico2_serial_console`

## Dependency

The console sketches depend on the SLMP minimal library:

- GitHub: <https://github.com/fa-yoshinobu/plc-comm-slmp-cpp-minimal>

PlatformIO uses the library directly from GitHub in `platformio.ini`.

## Quick Start

```bash
pio run -e m5stack-atom-console
pio run -e wiznet_6300_evb_pico2
```

For the W6300 console CLI:

```bash
python scripts/w6300_console_cli.py --help
```
