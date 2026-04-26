# Hardware Setup

## Bill of Materials

| Qty | Part | Notes |
|-----|------|-------|
| 1 | Teensy 4.1 | Main MCU, 600 MHz Cortex-M7, FPU, plenty of RAM |
| 1 | PJRC Teensy Audio Adapter (Rev D) | SGTL5000 codec + SD card slot |
| 1 | 2.8" or 3.2" ILI9341 SPI LCD with XPT2046 touch | 320×240 landscape |
| 1 | microSD card | Any size; stores patches + calibration |
| 1 | 5-pin DIN MIDI jack + MIDI input opto-isolator circuit | 6N138 or similar |
| 1 | USB MIDI-capable host (optional) | For USB MIDI input |
| 1 | 3.3 V power regulation (via USB or dedicated) | Teensy USB is fine for prototyping |
| — | Jumper wires, perf board or custom PCB | |

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
| RST    | — | Tied to 3.3 V via 10 kΩ pull-up (not driven by MCU) |

### Touch Controller (XPT2046)

| Signal  | Teensy Pin |
|---------|-----------|
| MOSI/MISO/SCK | Shared main SPI (11/12/13) |
| T_CS    | 41 |
| T_IRQ   | 2  |

### Audio Adapter

The PJRC audio adapter uses fixed pins:

| Signal       | Teensy Pin |
|--------------|-----------|
| I²S BCLK     | 21 |
| I²S LRCLK    | 20 |
| I²S TX       | 7  |
| I²S RX       | 8  |
| I²C SDA (SGTL5000) | 18 |
| I²C SCL (SGTL5000) | 19 |
| MCLK         | 23 |

### SD Card (on audio adapter — NOT the Teensy built-in SD slot)

| Signal | Teensy Pin |
|--------|-----------|
| CS     | 10 |
| MOSI   | 7  |
| SCK    | 14 |
| MISO   | 12 |

> ⚠️ **Do not** use the Teensy 4.1's onboard SD card slot. All SD access in this
> project uses the audio adapter's SD slot, which shares the audio shield SPI bus.

### DIN MIDI

| Signal | Teensy Pin |
|--------|-----------|
| MIDI IN (RX)  | 25 (Serial6 RX) |
| MIDI OUT (TX) | 24 (Serial6 TX) — optional, not used yet |

---

## Wiring Diagram
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

- The ILI9341 draws ~80 mA with backlight on. USB power from the Teensy is fine for prototyping.
- For field use, consider a dedicated 5 V 1 A supply.
- The audio adapter draws additional current when driving headphones; keep this in mind if batteries are involved.

---

## Future Expansion

Provisions in the code exist (but no hardware is wired yet) for:

- **Rotary encoders** — future hands-on control of parameters
- **Hardware buttons** — for tab navigation and patch recall
- **LED indicators** — per-voice activity, arpeggiator clock

See [`ROADMAP.md`](ROADMAP.md).
