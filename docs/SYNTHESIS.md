# Synthesis Engine Internals

## Voice structure

Each `Voice` is a conceptual oscillator-filter-amp triple plus envelopes:

```
   ┌──────────────────────────────────────────────┐
   │                    Voice                     │
   │                                              │
   │   saw (SAWTOOTH) ─┐                          │
   │   pulse (PW)    ──┼→ oscMix → filter (SVF LP)│
   │   sub (SQUARE½) ──┘                     │    │
   │                                         ↓    │
   │   ampEnv  ──────────────────────→  (gated)   │
   │                                              │
   │   filtEnv (software) → cutoff modulation     │
   │                                              │
   └──────────────────────────────────────────────┘
```

### Oscillators

- **Saw**: `AudioSynthWaveform` in `WAVEFORM_SAWTOOTH`
- **Pulse**: `AudioSynthWaveform` in `WAVEFORM_PULSE` with `pulseWidth()` control
- **Sub**: `AudioSynthWaveform` in `WAVEFORM_SQUARE`, tuned one octave below the base note

### Mixing

Per-voice, the three oscillator outputs are summed in an `AudioMixer4` with independent levels (saw, pulse, sub). The patch controls these at 0–1.

### Filter

A `AudioFilterStateVariable` in low-pass mode. Resonance is a Q value from 0.7 (no emphasis) to ~5 (near self-oscillation). The cutoff is driven by three stacked modulations:

1. **Base cutoff** from the patch
2. **LFO** (if routed to filter), applied multiplicatively
3. **Filter envelope**, applied as `(1 + envAmount × 4 × env)`

The software envelope estimator walks through A→D→S→R in real time based on elapsed `millis()`. This is simpler and uses less audio memory than wiring a separate envelope into the filter's control input.

### Amp envelope

A real Teensy `AudioEffectEnvelope` sits in the audio path. It applies the ADSR gain directly to the filter output. This ensures perfectly smooth attack and release regardless of LFO or filter-env activity.

## LFO

The LFO runs at **control rate** (1 kHz), not audio rate. Per tick:

1. Advance `lfoPhase` by `lfoRate / CONTROL_RATE_HZ`.
2. Compute the waveform:
   - Triangle (default)
   - Sine
   - Square
   - Sawtooth
3. Scale by `effectiveLfoDepth()` (patch depth blended with mod wheel).
4. Route to one of: pitch (±7 semitones), pulse-width (±0.4), filter cutoff (±2 octaves, exponential).
5. Fold pitch bend into the pitch axis.
6. Stash results atomically for the main loop to apply.

## Smoothing

When the user sweeps a slider or a CC changes, raw parameter jumps produce "zipper noise." To prevent this:

- `Voice::tickSmoothing()` runs every control tick (1 kHz).
- It uses a one-pole low-pass smoother:
```cpp
  smoothCutoff = smoothCutoff * 0.94 + target * 0.06;
```

  which gives a time constant of ~5 ms — inaudible as lag, but enough to erase stepping artifacts.

    Pulse width uses a slightly faster smoother (coefficient 0.90).


Glide / portamento

When glideMs > 0 in the patch:

- On note-on, the voice stores glideTarget = newFreq but keeps glideFreq at its previous value.
- Each control tick, glideFreq moves exponentially toward glideTarget:

```cpp
float tauTicks = glideMs;
float k = expf(-1.0f / tauTicks);
glideFreq = target + (glideFreq - target) * k;
```

- applyModulation() uses glideFreq instead of baseFreq.

This gives per-voice polyphonic glide — unlike monosynth portamento, each voice independently slides to its own new pitch.

Juno Chorus

See the full dedicated discussion below. The effect lives entirely inside a custom AudioEffectJunoChorus block.

Signal flow

```
Input (mono) ─→ Pre-LPF ─→ + ─→ Delay Line L ─→ Post-LPF ─┬─→ Output L
                           ↑                              │
                           └─── Feedback × 0.12 ──────────┘

                  (parallel)  Delay Line R (anti-phase LFO)
```

Characteristics

Element 	Detail
LFO shape 	Sine (anti-corner-artifact)
LFO smoothing 	One-pole LP on the LFO itself (coeff 0.05) for silky motion
Pre-LPF 	One-pole ~6 kHz (coeff 0.35) — mimics BBD input bandwidth
Post-LPF 	One-pole ~4.5 kHz (coeff 0.28) — mimics BBD reconstruction
Feedback 	12% of post-filter output fed back into write stage — analog coloration
Delay lines 	2048 samples each, fractional reads with linear interpolation
Chorus I 	0.513 Hz, ±22 sample modulation
Chorus II 	0.863 Hz, ±36 sample modulation

The opposite-phase LFOs on L and R produce the characteristic stereo spread of the original Juno chorus.

Velocity routing

When a note arrives, the engine checks patch.velDest:

- VCA: velocity scales oscillator amplitudes (default, standard dynamic).
- CUTOFF: velocity temporarily boosts the voice's cutoff by up to 4× (at full velocity × amount).
- LFO: velocity is fed into the mod-wheel channel, bringing in the LFO modulation on hard hits.
- OFF: velocity is ignored; every note plays at full amplitude.

Velocity amount (velAmount) scales how strongly the destination responds.
