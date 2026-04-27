# MIDI

## Inputs (all three simultaneous)

| Source | Wire | Rate | Notes |
|--------|------|------|-------|
| **USB device** | Teensy micro-USB | USB 2.0 | Teensy enumerates as "Teensy MIDI" on the host computer |
| **USB host** | Teensy 5-pin host header | USB 2.0 | MIDI keyboard plugs directly into the Teensy |
| **DIN MIDI** | Serial6 (pins 24/25) | 31250 baud | Standard opto-isolator circuit required |

All three feed into the same `arp` -> `synth` pipeline identically.

## MIDI channel filter

Each patch has a `midiChannel` field:

- 0 = OMNI (accepts all channels)
- 1-16 = specific channel

Set per patch on the PERF tab, MIDI section. Applies to note-on, note-off, CC, and pitch bend messages from all three sources.

## Pitch Bend

- Range: +/- 2 semitones (`PITCH_BEND_RANGE_SEMI` in `MidiHandler.cpp`)
- Applied per-voice at control rate -- smooth and glitch-free
- Respects the per-patch MIDI channel filter

## Control Change map

| CC | Parameter | Range |
|----|-----------|-------|
| 1  | Mod wheel | 0-1 (blends with patch LFO depth) |
| 3  | LFO rate | 0.05 - 20 Hz |
| 5  | Glide time | 0 - 1000 ms |
| 9  | LFO depth | 0 - 1 |
| 20 | Saw level | 0 - 1 |
| 21 | Pulse level | 0 - 1 |
| 22 | Sub level | 0 - 1 |
| 64 | **Sustain pedal** | >=64 = on, <64 = off |
| 70 | Pulse width | 0.05 - 0.95 |
| 71 | Resonance | 0.7 - 5 |
| 72 | Amp release | 0 - 5000 ms |
| 73 | Amp attack  | 0 - 3000 ms |
| 74 | Filter cutoff | 40 - 8000 Hz |
| 75 | HPF cutoff | 20 - 1000 Hz |
| 76 | Amp decay | 0 - 3000 ms |
| 77 | Amp sustain | 0 - 1 |
| 79 | Filter env amount | 0 - 1 |
| 93 | Chorus mode | 0-42: OFF; 43-85: I; 86-127: II |
| 120| All sound off | Silences all voices |
| 123| All notes off | Releases all held notes |

### Customizing the CC map

Edit `onCC()` in both `MidiHandler.cpp` and `UsbHostMidi.cpp` to keep DIN, USB device, and USB host behavior consistent. Each `case` maps one CC number to one `synth.setParam(ParamId, value)` call. Add entries as needed.

## USB device name

Defined by the Teensy core as "Teensy MIDI" when `USB_MIDI_SERIAL` is the selected USB type. To change, edit `usb_desc.c` in the Teensy core (outside this project's scope).

## USB host compatibility

Most class-compliant USB MIDI devices work. Confirmed:

- Korg nanoKEY / microKEY series
- Arturia MiniLab / KeyLab
- Novation Launchkey Mini
- M-Audio Keystation series
- Akai LPK / MPK Mini

If your keyboard is "class compliant" (no driver install needed on PC/Mac), it will almost certainly work.

Some keyboards need to be plugged in after the Teensy boots, not before.

## Future MIDI features

Planned -- see [`ROADMAP.md`](ROADMAP.md):

- MIDI clock sync -> arpeggiator
- MIDI Learn (long-press slider, wiggle CC, persistent binding)
- SysEx patch dump/load
- Polyphonic aftertouch
