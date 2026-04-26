# Juno-106 Synthesizer Emulator for Teensy 4.1

A polyphonic software emulation of the classic Roland Juno-106 analog synthesizer,
running on a Teensy 4.1 with the PJRC Audio Adapter. Features a touchscreen UI,
patch storage on SD card, full MIDI (USB + DIN), a true stereo BBD-style chorus,
and a built-in arpeggiator.

![Juno-106 Emulator](docs/images/hero.jpg)
<!-- Add your own photo here -->

---

## ✨ Features

- **6-voice polyphony** (easily extendable to 16)
- **Juno-106-style voice architecture**
  - Sawtooth + Pulse + Sub oscillators with pulse-width control
  - State-variable low-pass filter with resonance
  - Global high-pass filter
  - Separate amp + filter envelopes (ADSR)
- **LFO** with triangle / sine / square / saw shapes, routable to pitch, PW, or filter
- **Velocity routing**: VCA, cutoff, or LFO depth, with per-patch amount
- **Polyphonic portamento / glide** (0–1000 ms)
- **Custom stereo Juno chorus** — two fractional-delay lines with opposite-phase sine LFOs, BBD coloration, pre/post high-cut
- **Arpeggiator** — UP / DOWN / UP-DOWN / RANDOM, 1–4 octaves
- **Pitch bend + mod wheel**
- **Touchscreen UI** with 6 tabbed pages (OSC / VCF / ENV / CHORUS / PERF / PATCH)
- **On-screen keyboard** for patch naming
- **Touch calibration wizard** (3-corner tap, SD-persisted)
- **32 patch slots** on the audio-shield SD card
- **10 factory patches** installed on first boot
- **USB MIDI + DIN MIDI** with a broad CC map

---

## 🎛️ Quick Start

1. Build the hardware — see [`docs/HARDWARE.md`](docs/HARDWARE.md).
2. Install PlatformIO and clone this repo.
3. Open in VS Code with the PlatformIO extension.
4. Build & upload:
   ```bash
   pio run -t upload
   ```
5. On first boot, the system will run touch calibration, then install 10 factory patches.
6. Plug in a MIDI keyboard (USB or DIN) and play.


Full build instructions in [docs/BUILD.md](docs/BUILD.md).

--- 

📚 Documentation index

| Document | Topic |
|----------|-------|
|[Hardware](docs/HARDWARE.md) | Wiring, pinout, BOM |
|[Build & Setup](dosc/BUILD.md) | PlatformIO config, first boot|
|[Architecture](docs/ARCHITECTURE.md) | Software structure and threading model|
|[User Interface](docs/UI.md) | Touch UI layout and interaction|
|[Synthesis Engine](docs/SYNTHESIS.md) | Voice, chorus, LFO internals|
|[MIDI](docs/MIDI.md) | MIDI routing and CC map|
|[Patches](docs/PATCHES.md) | Patch storage format, factory presets|
|[Touch Calibration](docs/CALIBRATION.md) | Wizard, persistence, adjustment|
|[Tuning Guide](docs/TUNING.md) | Tweaking sound and feel|
|[Troubleshooting](docs/TROUBLESHOOTING.md) | Common issues and fixes|
|[Roadmap](docs/ROADMAP.md) | Planned features|


---

🏗️ Project Status

| Area | Status |
|------|--------|
| Synth engine | ✅ 6-voice polyphonic, stable |
| UI | ✅ 6 tabbed pages, touch-driven |
| MIDI (USB + DIN) | ✅ Full note/CC/bend support |
| Chorus | ✅ Custom BBD-style stereo effect |
| Patch storage | ✅ SD-backed, 32 slots |
| Arpeggiator | ✅ 5 modes, octave range |
| Touch calibration | ✅ On-screen wizard |
| Hardware encoders | 🚧 Planned |
| MIDI clock sync | 🚧 Planned |

---

🎹 Credits

- Built on PJRC's Teensy Audio Library
- ILI9341 driver: PaulStoffregen/ILI9341_t3
- XPT2046 driver: PaulStoffregen/XPT2046_Touchscreen
- MIDI: FortySevenEffects Arduino MIDI Library
- Original Juno-106 design © Roland Corporation (this project is a clean-room software emulation, not affiliated)

---

📄 License

MIT License — see LICENSE
 (add your preferred license).