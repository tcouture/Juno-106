# Software Architecture

## High-level view

```
+----------------------------------------------------------+
|                       main                               |
|                                                          |
|   setup():                                               |
|     synth.begin()                                        |
|     midiHandler.begin()      (USB device + DIN)          |
|     hostMidi.begin()         (USB host)                  |
|     arp.begin()                                          |
|     patchManager.begin()                                 |
|     ui.begin()               (calibration if needed)     |
|                                                          |
|   loop():                                                |
|     midiHandler.update()    (USB device + DIN MIDI)      |
|     hostMidi.update()       (USB host MIDI)              |
|     synth.update()          (applies pending mods)       |
|     arp.tick(millis())                                   |
|     ui.update()             (touch + redraws)            |
+----------------------------------------------------------+
```

## Module responsibilities

| Module | File(s) | Role |
|--------|---------|------|
| `SynthEngine` | `SynthEngine.{h,cpp}` | Audio graph, voice management, patch application, LFO, pitch bend, mod wheel, sustain, channel filtering |
| `Voice` | `Voice.{h,cpp}` | One polyphonic voice: DCO + LPF + VCA + envelopes, smoothing, glide |
| `AudioEffectJunoChorus` | `AudioEffectJunoChorus.{h,cpp}` | Custom stereo BBD-style chorus audio block |
| `AudioEffectSoftClip` | `AudioEffectSoftClip.{h,cpp}` | Master drive/saturation stage |
| `MidiHandler` | `MidiHandler.{h,cpp}` | USB-device + DIN MIDI routing |
| `UsbHostMidi` | `UsbHostMidi.{h,cpp}` | USB host MIDI (keyboards plugged into Teensy) |
| `MidiActivity` | `MidiActivity.{h,cpp}` | Central activity tracker for header indicators |
| `Arpeggiator` | `Arpeggiator.{h,cpp}` | Held-note buffer + timed note stepping |
| `PatchManager` | `PatchManager.{h,cpp}` | SD card save/load/list/rename |
| `TouchCalibration` | `TouchCalibration.{h,cpp}` | 3-corner tap wizard + SD persistence |
| `OnScreenKeyboard` | `OnScreenKeyboard.{h,cpp}` | Modal keyboard for patch naming |
| `UI` | `UI.{h,cpp}` | All drawing, touch dispatch, page navigation, status header |
| `FactoryPatches` | `FactoryPatches.{h,cpp}` | One-shot installer for 22 starter presets |

## Threading / interrupt model

Three effectively concurrent contexts:

1. **Main loop** -- `loop()` runs tens of thousands of times per second, handling MIDI parsing (all three sources), UI drawing, and non-audio logic.
2. **Audio interrupt** -- The Teensy Audio library runs an ISR every 128-sample block (~2.9 ms at 44.1 kHz). This processes the entire audio graph (oscillators, filters, envelopes, chorus, soft clip).
3. **Control-rate ISR** -- `IntervalTimer` fires at 1 kHz (`CONTROL_RATE_HZ`). This advances the LFO, folds in pitch bend, ticks smoothing and glide, and stashes new modulation values in a flag-guarded buffer. NEVER touches audio objects directly.

### Producer/consumer mod updates

The control ISR only computes new mod values and sets `modDirty = true`. `synth.update()` -- called from the main loop -- reads them atomically and applies them to the audio objects. This avoids ISR-vs-audio races.

## Audio graph

```
Voice 0 -+
       Voice 1 -+
       Voice 2 -+
       Voice 3 -+-> subMix1 -+
       Voice 4 -+            |
       Voice 5 -+            +-> preHPF -> HPF -+-> driveL -> I2S L
       Voice 6 -+            |                  |
       Voice 7 -+            +-> subMix2 -+     +-> JunoChorus
          (...up to 16)      +-> subMix3 -+     |   driveR -> I2S R
                             +-> subMix4 -+     |
                                                +-> dry+wet mix
```

Each voice is a chain of:

```
saw   -+
pulse -+-> oscMix -> filter (SVF LP) -> ampEnv -> voice out
sub   -+
```

The filter envelope is NOT in the audio graph -- it's estimated in software and
applied to the cutoff at control rate.

## Memory footprint

- `AudioMemory(240)` at 8 voices.
- `AudioEffectJunoChorus`: two 2048xint16 delay buffers = 8 kB.
- `PatchData`: ~88 bytes; 32 slots = ~2.8 kB on SD.
- Factory patch set (22 patches) = ~2 kB on SD.
