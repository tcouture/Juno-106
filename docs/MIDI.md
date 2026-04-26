# MIDI

## Inputs

| Source | Wire | Rate | Notes |
|--------|------|------|-------|
| **USB MIDI** | Teensy USB port | — | Auto-detected by hosts as `Teensy MIDI` |
| **DIN MIDI** | Serial6 (pins 24/25) | 31250 baud | Requires standard opto-isolator circuit |

Both inputs are merged; any note or CC on either source drives the synth.

## MIDI channel

Currently **MIDI_CHANNEL_OMNI** — all channels trigger the synth. Future versions will add per-channel routing.

## Note messages

- Routed through the arpeggiator (which passes through transparently when OFF).
- Velocity is handled per-patch (see `velDest`).
- Note-off (or note-on with velocity 0) releases the voice.

## Pitch Bend

- Range: ±2 semitones (`PITCH_BEND_RANGE_SEMI` in `MidiHandler.cpp`).
- Applied per-voice at control rate — smooth and glitch-free.

## Control Change map

| CC | Parameter | Range Mapping |
|----|-----------|--------------|
| 1  | Mod wheel | 0–1 (blends with patch LFO depth) |
| 3  | LFO rate | 0.05 – 20 Hz |
| 5  | Glide time | 0 – 1000 ms |
| 9  | LFO depth | 0 – 1 |
| 20 | Saw level | 0 – 1 |
| 21 | Pulse level | 0 – 1 |
| 22 | Sub level | 0 – 1 |
| 70 | Pulse width | 0.05 – 0.95 |
| 71 | Resonance | 0.7 – 5 |
| 72 | Amp release | 0 – 5000 ms |
| 73 | Amp attack  | 0 – 3000 ms |
| 74 | Filter cutoff | 40 – 8000 Hz |
| 75 | HPF cutoff | 20 – 1000 Hz |
| 76 | Amp decay | 0 – 3000 ms |
| 77 | Amp sustain | 0 – 1 |
| 79 | Filter env amount | 0 – 1 |
| 93 | Chorus mode | 0–42: OFF; 43–85: I; 86–127: II |
| 120| All sound off | Silences all voices |
| 123| All notes off | Releases all held notes |

### Customizing the CC map

Edit `MidiHandler.cpp`'s `onCC()` function. Each `case` maps one CC number to one `synth.setParam(ParamId, value)` call. Want pitch bend range on CC 101? Add:

```cpp
case 101: synth.setParam(ParamId::PitchBendRange, val / 127.0f * 24.0f); break;
````

(You'd also need to add PitchBendRange to ParamId and handle it in setParam.)

## USB MIDI Device Name

Defined by the Teensy core. You'll see "Teensy MIDI" on your host. To change, edit usb_desc.c in the Teensy core — outside the scope of this project.

## Future MIDI features

Planned (see ROADMAP.md):

- MIDI channel selection per synth mode
- MIDI clock input → arpeggiator sync
- SysEx patch dump/load
- MIDI Learn for touch sliders
