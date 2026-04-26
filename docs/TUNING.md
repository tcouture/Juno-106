# Tuning & Tweaking Guide

This document covers the "knobs under the hood" — compile-time constants and
code-level adjustments to shape the instrument's feel and sound.

## Chorus character

In `AudioEffectJunoChorus.h`:

```cpp
static constexpr float LFO_SMOOTH_COEFF  = 0.05f;
static constexpr float PRE_HICUT_COEFF   = 0.35f;
static constexpr float OUT_HICUT_COEFF   = 0.28f;
static constexpr float FEEDBACK_AMOUNT   = 0.12f;
```


Constant 	Effect 	Try
LFO_SMOOTH_COEFF 	LFO roundness 	0.02 (very smooth) – 0.2 (closer to raw sine)
PRE_HICUT_COEFF 	How dark the chorus sounds 	0.2 (dark) – 0.6 (bright)
OUT_HICUT_COEFF 	Wet signal brightness 	0.15 (very warm) – 0.5 (airy)
FEEDBACK_AMOUNT 	Coloration depth 	0 (clean) – 0.2 (edge of flange)

To get a pure clean digital chorus (no BBD emulation):
```cpp
PRE_HICUT_COEFF = 1.0f;
OUT_HICUT_COEFF = 1.0f;
FEEDBACK_AMOUNT = 0.0f;
```

Smoothing time constants

In Voice.cpp:
```cpp
const float cutCoeff = 0.94f;  // ~5 ms
const float pwCoeff  = 0.90f;  // ~3 ms
```

Smaller values = slower = more zipper-free but more "lag." If you notice sliders feeling slow to respond, raise these to 0.96–0.98. If you still hear zipper on fast sweeps, lower to 0.90.

LFO depth scaling

In SynthEngine::controlTick():

    Pitch: lfoValue * depth * 7.0f semitones max
    PW: lfoValue * depth * 0.4f offset max
    Filter: 2 ^ (lfoValue * depth * 2.0f) (±2 octaves max)


Adjust the constants (7, 0.4, 2.0) for wider or narrower modulation ranges.

Pitch bend range

In MidiHandler.cpp:

```cpp
static constexpr float PITCH_BEND_RANGE_SEMI = 2.0f;
```

Change to 12.0f for a full octave bend.

Arpeggiator rate range

In Arpeggiator::setRateHz():

```cpp
if (hz < 0.5f)  hz = 0.5f;
if (hz > 16.0f) hz = 16.0f;
```

Polyphony

In Config.h:

```cpp
#define MAX_VOICES 6
```

You can raise up to MAX_VOICES_LIMIT (16). Consider also bumping AudioMemory(...) in SynthEngine.cpp:

Voices 	Suggested AudioMemory
6 	200
8 	240
12 	320
16 	400

Voice amplitude

In Voice::noteOn():

```cpp
saw_->amplitude(0.6f * amp);
pulse_->amplitude(0.6f * amp);
sub_->amplitude(0.5f * amp);
```

These are the per-oscillator levels before the voice mix. If you notice clipping at full polyphony, lower to 0.5f / 0.4f. If the synth is too quiet, raise to 0.7f (watch for clipping).

Master output

masterMix is set to 0.25f per voice sub-mixer. That's headroom for 6 voices. Adjust in SynthEngine::begin():

```cpp
for (int i = 0; i < 4; i++) {
    subMix1.gain(i, 0.25f);  // ← here
    ...
}
```

SGTL5000 output level

The codec's headphone amplifier level:

```cpp
codec.volume(0.6f);  // 0..1
```

Envelope snappiness

In Voice::init() defaults (and more importantly in each patch), the A/D/S/R parameters are in milliseconds. 

For very tight percussion:

- A: 1–3 ms
- D: 50–100 ms
- S: 0
- R: 30–80 ms

For pads:

- A: 500–1500 ms
- D: 1000–2500 ms
- S: 0.7–0.9
- R: 1500–3000 ms
