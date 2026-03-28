# Console Examples

This repository keeps the interactive console programs that were split out of `plc-comm-slmp-cpp-minimal`.

## Included Examples

- `atom_matrix_serial_console`
- `w6300_evb_pico2_serial_console`

## Build

```bash
pio run -e m5stack-atom-console
pio run -e wiznet_6300_evb_pico2
```

## W6300 PC CLI

```bash
python scripts/w6300_console_cli.py --help
```
