# Wio Terminal USB2UART

USB to UART bridge for Wio Terminal, built with Zephyr RTOS.

USB CDC ACM virtual serial port on one side, SERCOM3 UART (left Grove connector, PA16/PA17) on the other. Real-time status and data display on the built-in ILI9341 320x240 LCD.

## Hardware

- Wio Terminal (ATSAMD51P19, Cortex-M4F 120MHz)
- Left Grove connector: TX=PA16, RX=PA17 (SERCOM3)

## Build

```bash
west build -b wio_terminal --pristine
```

## Flash

Copy `build/zephyr/zephyr.uf2` to the Wio Terminal USB mass storage drive (double-tap reset to enter bootloader).

## Usage

1. Connect Grove UART cable to left Grove port
2. Connect Wio Terminal to host PC via USB
3. Open a serial terminal on the host (e.g., minicom, PuTTY)
4. Set baudrate/data bits/stop bits/parity in the terminal — Wio Terminal auto-syncs to UART
5. Data flows bidirectionally between USB CDC ACM and Grove UART
6. Screen shows: baudrate config, connection status, TX/RX data in hex+ASCII

## Configuration

- `CONFIG_USB2UART_DISPLAY_REFRESH_MS` — display refresh interval (default: 100ms)
- `CONFIG_USB2UART_MAX_DATA_LINES` — max hex lines per direction (default: 128)
