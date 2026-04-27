# Synthesis Engine Internals

## Voice structure

Each `Voice` is a conceptual oscillator-filter-amp triple plus envelopes:

```
   +----------------------------------------------+
   |                    Voice                     |
   |                                              |
   |   saw (BANDLIMIT_SAWTOOTH) -+                |
   |   pulse (BANDLIMIT_PULSE) --+-> mix -> LPF   |
   |   sub (BANDLIMIT_SQUARE)  --+                |
   |                                              |
   |   ampEnv  -> gates the filter output         |
   |   filtEnv (software) -> cutoff modulation    |
   +----------------------------------------------+
```

### Oscillators (band-limited)

All three oscillators use band-limited waveform synthesis:

- **Saw**: `WAVEFORM_BANDLIMIT_SAWTOOTH`
- **Pulse**: `WAVEFORM_BANDLIMIT_PULSE` with runtime `pulseWidth()` control
- **Sub**: `WAVEFORM_BANDLIMIT_SQUARE`, tuned one octave below the base note

Band-limited synthesis eliminates the high-frequency aliasing artifacts present
in naive waveform generation. At the cost of ~2x CPU per oscillator, you get
clean, alias-free tone across the full keyboard range.

### Mixing

Per-voice, the three oscillator outputs sum via `AudioMixer4` with independent
levels set by the patch's `sawLevel`, `pulseLevel`, `subLevel` fields.

### Filter

`AudioFilterStateVariable` in low-pass mode. Resonance goes from 0.7 (no emphasis) to ~5 (near self-oscillation). Cutoff is driven by three stacked modulations:

1. Base cutoff from the patch
2. LFO (if routed to filter), applied multiplicatively
3. Filter envelope (software estimator), applied as `(1 + envAmount * 4 * env)`

### Amp envelope

A real Teensy `AudioEffectEnvelope` sits in the audio path, applying ADSR gain directly to the filter output.

## LFO

Runs at control rate (1 kHz), not audio rate. Per tick:

1. Advance `lfoPhase` by `lfoRate / CONTROL_RATE_HZ`.
2. Compute the waveform (triangle / sine / square / saw).
3. Scale by `effectiveLfoDepth()` = patch depth blended with mod wheel.
4. Route to one destination:
   - Pitch: +/- 7 semitones max
   - Pulse-width: +/- 0.4 offset max
   - Filter cutoff: +/- 2 octaves (exponential)
5. Fold pitch bend into the pitch axis.
6. Stash results atomically for `synth.update()` to consume in the main loop.

## Smoothing

`Voice::tickSmoothing()` runs every control tick. One-pole low-pass filter:

```
smoothCutoff = smoothCutoff * 0.94 + target * 0.06
```

Time constant ~5 ms -- inaudible as lag but kills zipper noise on sweeps.

## Glide / portamento

When `glideMs > 0`:

- On note-on, `glideTarget = newFreq` but `glideFreq` stays at its previous value.
- Each control tick, `glideFreq` moves exponentially toward `glideTarget`.
- `applyModulation()` uses `glideFreq` instead of `baseFreq`.

Result: per-voice polyphonic glide -- each voice independently slides to its own new pitch.

## Juno Chorus (custom AudioEffectJunoChorus)

### Signal flow

Signal flow

```
Input (mono) ─→ Pre-LPF ─→ + ─→ Delay Line L ─→ Post-LPF ─┬─→ Output L
                           ↑                              │
                           └─── Feedback × 0.12 ──────────┘

                  (parallel)  Delay Line R (anti-phase LFO)
```

### Characteristics

| Element | Detail |
|---------|--------|
| LFO shape | Sine (corner-artifact-free) |
| LFO smoothing | One-pole LP on the LFO itself (coeff 0.05) |
| Pre-LPF | One-pole ~6 kHz (coeff 0.35) -- BBD input bandwidth sim |
| Post-LPF | One-pole ~4.5 kHz (coeff 0.28) -- BBD reconstruction sim |
| Feedback | 12% of post-filter output -- analog coloration |
| Delay lines | 2048 samples each, fractional reads with linear interpolation |
| Chorus I  | 0.513 Hz, +/- 22 sample modulation |
| Chorus II | 0.863 Hz, +/- 36 sample modulation |

Per-patch `chorusRate` and `chorusDepth` override the presets when set.

## Master drive (AudioEffectSoftClip)

Stereo soft-clip stage at the master output:

```
y = tanh(x * drive) * outGain
```

Where `outGain = 1 / sqrt(drive)` so perceived level stays roughly constant as drive increases.

| Drive | Character |
|-------|-----------|
| 1.0 (default) | Clean, effectively transparent |
| 2-3 | Warm thickening -- great on pads/basses |
| 5+ | Overt overdrive -- saturated leads, gritty basses |
| 8+ | Aggressive distortion (effect territory) |

## Velocity routing

When a note arrives, the engine checks `patch.velDest`:

- **VCA**: velocity scales oscillator amplitudes (default).
- **CUTOFF**: velocity temporarily boosts the voice's cutoff by up to 4x (at full velocity x amount).
- **LFO**: velocity feeds into the mod-wheel channel, bringing in LFO modulation on hard hits.
- **OFF**: velocity ignored; every note plays at full amplitude.

`velAmount` scales how strongly the destination responds.

## Sustain pedal

CC 64 controls `synth.setSustain(bool)`:

- While sustained, note-off events are intercepted and tracked in a held-notes bitmap.
- When the pedal releases (val < 64), all held notes are released immediately.
- Re-striking a note while sustained cancels its held status for that voice.

## MIDI channel filter

`patch.midiChannel`:

- 0 = OMNI (accepts all channels)
- 1-16 = specific channel

The filter is checked in all three MIDI handlers (USB device, USB host, DIN) for note-on, note-off, CC, and pitch bend.
