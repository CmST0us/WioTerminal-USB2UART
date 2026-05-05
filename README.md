# Wio Terminal USB2UART

A USB-to-UART bridge for the [Wio Terminal](https://www.seeedstudio.com/Wio-Termial-p-4509.html), built with [Zephyr RTOS](https://www.zephyrproject.org/).

Turns the Wio Terminal into a full-featured USB serial adapter — plug into USB, connect a Grove cable, and you have a transparent serial bridge. The built-in 320×240 LCD shows live connection status, baudrate, and Grove wiring reference.

## Features

- Transparent USB CDC ACM ↔ UART bridge (bidirectional)
- Baudrate auto-synced from USB host via `SET_LINE_CODING` (up to 1.5 Mbps)
- Composite USB device: bridge port + console/shell on separate CDC ACM interfaces
- Nothing Design UI on ILI9341 LCD: hero baudrate, connection indicator, wire color legend
- Zephyr `uart_bridge` driver handles ring buffers, IRQ forwarding, and line coding sync

## Hardware

| | |
|---|---|
| **MCU** | ATSAMD51P19 (Cortex-M4F @ 120 MHz, 192 KB RAM, 512 KB Flash) |
| **Display** | ILI9341 320×240 SPI LCD |
| **UART** | SERCOM3 on left Grove connector (PA17=TX, PA16=RX) |
| **USB** | Native SAMD51 USB 2.0 Full-Speed |

> **Note:** SERCOM3 is shared with the onboard LIS3DH accelerometer and ATECC608A crypto chip (I2C on the same SERCOM). This overlay reconfigures SERCOM3 as UART, so those I2C devices become unavailable.

## Grove Wiring

The left Grove connector maps to the following SERCOM3 pads:

| Wire Color | Signal | Pin |
|------------|--------|-----|
| White | RX | PA16 (SERCOM3 PAD1) |
| Yellow | TX | PA17 (SERCOM3 PAD0) |
| Red | VCC | — |
| Black | GND | — |

## Prerequisites

- [Zephyr SDK](https://docs.zephyrproject.org/latest/develop/getting_started/index.html) with west toolchain installed
- Zephyr RTOS v4.3.0+ (for `USB_DEVICE_STACK_NEXT` and `uart_bridge` support)
- Wio Terminal with UF2 bootloader (factory default)

## Build

```bash
# Clone
git clone https://github.com/CmST0us/WioTerminal-USB2UART.git
cd WioTerminal-USB2UART

# Build
west build -b wio_terminal --pristine
```

The build output is at `build/zephyr/zephyr.uf2`.

## Flash

1. Double-tap the reset button on the Wio Terminal to enter UF2 bootloader mode
2. The device appears as a USB mass storage drive
3. Copy the firmware:

```bash
cp build/zephyr/zephyr.uf2 /path/to/wio_terminal_drive/
```

The device automatically reboots into the new firmware.

## Usage

1. Connect a Grove UART cable to the **left** Grove port on the Wio Terminal
2. Connect the Wio Terminal to your host PC via USB-C
3. The device enumerates as a composite USB device with two serial ports:
   - **Bridge port** — the USB-to-UART bridge (use this one)
   - **Console port** — Zephyr shell for debugging
4. Open the bridge port with your preferred serial terminal:

```bash
# Linux/macOS
minicom -D /dev/ttyACM0 -b 115200

# Or with picocom
picocom /dev/ttyACM0 -b 115200
```

5. Set baudrate, data bits, stop bits, and parity in the terminal — the Wio Terminal automatically syncs these settings to the UART side via `SET_LINE_CODING`
6. Data flows transparently in both directions

### Display

The LCD shows:

- **Baudrate** — large hero number, updates when the host changes line coding
- **Connection dot** — green when a host application has the port open
- **Config line** — current data bits, parity, stop bits (e.g. `8 N 1`)
- **GROVE LEFT** — wire color legend for quick cable identification
- **USB** — USB interface label

## Configuration

Custom Kconfig options:

| Option | Default | Description |
|--------|---------|-------------|
| `CONFIG_USB2UART_DISPLAY_REFRESH_MS` | 100 | Display refresh interval (ms) |

Key buffer settings in `prj.conf` (tune for stability at high baudrates):

| Option | Default | Description |
|--------|---------|-------------|
| `CONFIG_UART_BRIDGE_BUF_SIZE` | 2048 | Per-direction ring buffer size |
| `CONFIG_UDC_BUF_COUNT` | 32 | USB endpoint buffer count |
| `CONFIG_UDC_BUF_POOL_SIZE` | 4096 | USB endpoint buffer pool size |

## Project Structure

```
.
├── boards/
│   └── wio_terminal.overlay   # Devicetree: SERCOM3 UART + CDC ACM + uart-bridge
├── src/
│   ├── main.c                 # USB init, bridge monitor, display loop
│   ├── bridge_monitor.c       # Reads UART config from CDC ACM
│   ├── bridge_monitor.h
│   ├── display.c              # LVGL Nothing Design UI
│   └── display.h
├── CMakeLists.txt
├── Kconfig
└── prj.conf
```

## License

Apache-2.0
