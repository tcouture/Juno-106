# Hardware Setup

## Bill of Materials

| Qty | Part | Notes |
|-----|------|-------|
| 1 | Teensy 4.1 | Main MCU, 600 MHz Cortex-M7, FPU, plenty of RAM |
| 1 | PJRC Teensy Audio Adapter (Rev D) | SGTL5000 codec + SD card slot |
| 1 | 2.8" or 3.2" ILI9341 SPI LCD with XPT2046 touch | 320x240 landscape |
| 1 | microSD card | Any size; stores patches + calibration |
| 1 | 5-pin DIN MIDI jack + opto-isolator circuit | 6N138 or similar |
| 1 | USB-A receptacle (for USB host) | Wired to Teensy USB host header |
| 1 | PJRC Teensy 4.1 USB Host Cable (optional) | Pre-made with USB-A socket |
| 1 | 3.3 V power (via USB or dedicated) | Teensy USB is fine for prototyping |
| - | Jumper wires, perf board or custom PCB | |

---

## Pin Assignments

All pin numbers are Teensy 4.1 digital pins.

### Display (ILI9341)

| Signal | Teensy Pin | Notes |
|--------|-----------|-------|
| MOSI   | 11 | Shared main SPI bus |
| MISO   | 12 | Shared main SPI bus |
| SCK    | 13 | Shared main SPI bus |
| CS     | 40 | |
| DC     | 9  | |
| RST    | -  | Tied to 3.3 V via 10 kohm pull-up |

### Touch Controller (XPT2046)

| Signal  | Teensy Pin |
|---------|-----------|
| MOSI/MISO/SCK | Shared main SPI (11/12/13) |
| T_CS    | 41 |
| T_IRQ   | 2  |

### Audio Adapter

Fixed pins used by the PJRC audio adapter:

| Signal       | Teensy Pin |
|--------------|-----------|
| I2S BCLK     | 21 |
| I2S LRCLK    | 20 |
| I2S TX       | 7  |
| I2S RX       | 8  |
| I2C SDA      | 18 |
| I2C SCL      | 19 |
| MCLK         | 23 |

### SD Card (on audio adapter, NOT the Teensy built-in SD slot)

| Signal | Teensy Pin |
|--------|-----------|
| CS     | 10 |
| MOSI   | 7  |
| SCK    | 14 |
| MISO   | 12 |

> WARNING: Do NOT use the Teensy 4.1's onboard SD card slot. All SD access in this
> project uses the audio adapter's SD slot.

### DIN MIDI

| Signal | Teensy Pin |
|--------|-----------|
| MIDI IN (RX)  | 25 (Serial6 RX) |
| MIDI OUT (TX) | 24 (Serial6 TX) — optional, not used yet |

### USB Host (for plugging a MIDI keyboard directly into the Teensy)

Teensy 4.1 has a dedicated 5-pin USB host header along the board edge.
You need to solder a 5-pin male header and wire a USB-A socket to it.

| Teensy 4.1 USB Host Pin | Signal | USB-A Pin |
|---|---|---|
| +5V | VBUS | 1 (red) |
| D-  | Data- | 2 (white) |
| D+  | Data+ | 3 (green) |
| GND | GND   | 4 (black) |

PJRC sells a pre-made cable (SKU: Teensy 4.1 USB Host Cable) that plugs into
this header and terminates in a USB-A socket.

**Power note**: Bus-powered MIDI keyboards (50-200 mA) work fine when the Teensy
is powered from a >=1 A supply. Self-powered keyboards only need D+ / D- / GND.

---

## Wiring Overview

```
                  ┌─────────────────────┐
                  │     Teensy 4.1      │
                  │                     │
  DIN MIDI IN ────│ 25 (Serial6 RX)     │
                  │ 24 (Serial6 TX) ──→ │──── (optional MIDI OUT)
                  │                     │
  Touch IRQ ──────│ 2                   │
                  │                     │
  Display DC ─────│ 9                   │
  SD CS ──────────│ 10 (audio shield)   │
  Display MOSI ───│ 11 ──┐              │
  Display MISO ───│ 12 ──┼─── SPI bus   │
  Display SCK ────│ 13 ──┘              │
                  │                     │
  SD SCK ─────────│ 14 (audio shield)   │
                  │                     │
  Display CS ─────│ 40                  │
  Touch CS ───────│ 41                  │
                  │                     │
  Audio Adapter   │ 7, 8, 18-23 (fixed) │
                  └─────────────────────┘
```
---

## Power Considerations

- The ILI9341 draws ~80 mA with backlight on.
- Bus-powered USB MIDI keyboards add 50-200 mA.
- A dedicated 5 V / 1-2 A supply is recommended for reliability with host keyboards.

---

## Future Expansion

The code has provisions (but no hardware yet) for:

- Rotary encoders for primary parameters
- Hardware buttons for tab navigation and patch recall
- LED indicators

See [`ROADMAP.md`](ROADMAP.md).