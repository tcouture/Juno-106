# Software Architecture

## High-level view

```
┌─────────────────────────────────────────────────────┐
│                       main                          │
│                                                     │
│   setup():                                          │
│     synth.begin()                                   │
│     midiHandler.begin()                             │
│     arp.begin()                                     │
│     patchManager.begin()                            │
│     ui.begin()           (calibration if needed)    │
│                                                     │
│   loop():                                           │
│     midiHandler.update() → arp → synth              │
│     synth.update()       (applies LFO mods)         │
│     arp.tick(millis())                              │
│     ui.update()                                     │
└─────────────────────────────────────────────────────┘
```

## Module responsibilities

| Module | File(s) | Role |
|--------|---------|------|
| `SynthEngine` | `SynthEngine.{h,cpp}` | Audio graph, voice management, patch application, LFO/pitch-bend/mod-wheel |
| `Voice` | `Voice.{h,cpp}` | One polyphonic voice: DCO + LPF + VCA + envelopes, smoothing, glide |
| `AudioEffectJunoChorus` | `AudioEffectJunoChorus.{h,cpp}` | Stereo chorus audio block |
| `MidiHandler` | `MidiHandler.{h,cpp}` | USB + DIN MIDI routing |
| `Arpeggiator` | `Arpeggiator.{h,cpp}` | Held-note buffer + timed note stepping |
| `PatchManager` | `PatchManager.{h,cpp}` | SD card save/load/list/rename |
| `TouchCalibration` | `TouchCalibration.{h,cpp}` | 3-corner tap wizard + SD persistence |
| `OnScreenKeyboard` | `OnScreenKeyboard.{h,cpp}` | Modal keyboard for patch naming |
| `UI` | `UI.{h,cpp}` | All drawing, touch dispatch, page navigation |
| `FactoryPatches` | `FactoryPatches.{h,cpp}` | One-shot installer for starter presets |

## Threading / interrupt model

The Teensy 4.1 runs three effectively concurrent contexts:

1. **Main loop** — `loop()` runs as fast as possible (~tens of thousands of times per second), handling MIDI parsing, UI drawing, and non-audio logic.
2. **Audio interrupt** — The Teensy Audio library runs an ISR every 128-sample block (~2.9 ms at 44.1 kHz). This processes the entire audio graph (oscillators, filters, envelopes, chorus).
3. **Control-rate ISR** — `IntervalTimer` fires at 1 kHz (`CONTROL_RATE_HZ`). This advances the LFO, folds in pitch bend, ticks smoothing, and stashes new modulation values in a flag-guarded buffer. **It never touches audio objects directly.**

### Why the split?

Audio library setters (like `filter.frequency()`) aren't safe to call from an `IntervalTimer` ISR because they can race with the audio ISR's internal state updates. To avoid glitches:

- The control ISR only *computes* new mod values and sets `modDirty = true`.
- `synth.update()` — called from the main loop — reads the flagged values atomically and applies them to the audio objects.

This is a classic producer/consumer pattern. See [`SYNTHESIS.md`](SYNTHESIS.md) for more detail.

## Audio graph

```
Voice 0 ─┐
        Voice 1 ─┤
        Voice 2 ─┤
        Voice 3 ─┼─→ subMix1 ─┐
        Voice 4 ─┤            │
        Voice 5 ─┘            ├─→ preHPF ─→ HPF ─┬─→ dry L ─→ mixL ─→ I2S L
          (…up to Voice 15)   │                  │                          
                              ├─→ subMix2 ─┐    ├─→ JunoChorus (L) ─→ wet L
                              ├─→ subMix3 ─┤    │   JunoChorus (R) ─→ wet R
                              └─→ subMix4 ─┘    └─→ dry R ─→ mixR ─→ I2S R
```

Each voice is a chain of:

```
saw ─┐
pulse┼─→ oscMix ─→ filter (SVF LP) ─→ ampEnv ─→ voice out
sub ─┘
```

The filter envelope is *not* in the audio graph — it's estimated in software and
applied to the cutoff at control rate. This simplifies the graph and uses less audio memory.

## Memory footprint

- `AudioMemory(200)` allocates ~200 audio blocks (each is 256 bytes).
- `AudioEffectJunoChorus` holds two 2048×int16 delay buffers = 8 kB.
- `PatchData` is ~80 bytes; 32 slots = ~2.5 kB on SD.
- Factory patch set (10 patches) is ~800 bytes on SD.
