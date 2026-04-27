# Tuning & Tweaking Guide

This document covers the "knobs under the hood" — compile-time constants and
code-level adjustments to shape the instrument's feel and sound.

## Polyphony

In `Config.h`:

```cpp
#define MAX_VOICES 8
```
Scales up to 16. Bump `AudioMemory()` in `SynthEngine.cpp` accordingly:

| Voices | Suggested AudioMemory |
|--------|----------------------|
| 6 | 200 |
| 8 | 240 |
| 12 | 320 |
| 16 | 400 |

## Chorus character

In `AudioEffectJunoChorus.h`:

```cpp
static constexpr float LFO_SMOOTH_COEFF  = 0.05f;
static constexpr float PRE_HICUT_COEFF   = 0.35f;
static constexpr float OUT_HICUT_COEFF   = 0.28f;
static constexpr float FEEDBACK_AMOUNT   = 0.12f;
```

| Constant | Effect | Range to try |
|----------|--------|-----|
| `LFO_SMOOTH_COEFF` | LFO roundness | 0.02 (very smooth) - 0.2 |
| `PRE_HICUT_COEFF`  | Chorus darkness | 0.2 (dark) - 0.6 (bright) |
| `OUT_HICUT_COEFF`  | Wet brightness | 0.15 (warm) - 0.5 (airy) |
| `FEEDBACK_AMOUNT`  | Coloration depth | 0 (clean) - 0.2 (flange edge) |

To get a pure clean digital chorus (no BBD emulation):
```cpp
PRE_HICUT_COEFF = 1.0f;
OUT_HICUT_COEFF = 1.0f;
FEEDBACK_AMOUNT = 0.0f;
```

## Soft clip drive

In `AudioEffectSoftClip::setDrive()`: range clamped to 1-16. Internally `outGain = 1/sqrt(drive)` keeps perceived level flat.

If you want harder or softer clipping behavior, replace `tanhf()` with alternative curves (e.g., polynomial soft clip, or asymmetric clipper for tube-amp vibe).

## Smoothing time constants

In Voice.cpp:
```cpp
const float cutCoeff = 0.94f;  // ~5 ms
const float pwCoeff  = 0.90f;  // ~3 ms
```

Smaller = slower (less zipper, more lag). Try 0.96-0.98 if UI feels too snappy. Raise to 0.90 if fast sweeps still zipper.

## LFO depth scaling

In `SynthEngine::controlTick()`:

- Pitch: `lfoValue * depth * 7.0f` semitones max
- PW: `lfoValue * depth * 0.4f` offset max
- Filter: `2 ^ (lfoValue * depth * 2.0f)` +/- 2 octaves max

Adjust constants 7, 0.4, 2.0 for wider/narrower modulation ranges.

## Pitch bend range

In `MidiHandler.cpp` and `UsbHostMidi.cpp`:

```cpp
static constexpr float PITCH_BEND_RANGE_SEMI = 2.0f;
```

Change to 12.0 for full-octave bend. Keep both files in sync.

## Arpeggiator rate range

In `Arpeggiator::setRateHz()`:

```cpp
if (hz < 0.5f)  hz = 0.5f;
if (hz > 16.0f) hz = 16.0f;
```

## Voice amplitude

In `Voice::noteOn()`:

```cpp
saw_->amplitude(0.6f * amp);
pulse_->amplitude(0.6f * amp);
sub_->amplitude(0.5f * amp);
```

Lower to 0.5/0.4 if clipping at full polyphony. Raise to 0.7 for more level.

## Master output

Per-voice subMixer gains in `SynthEngine::begin()` default to 0.25f. Together with `drive` and `codec.volume(0.6f)` these set the output level. The soft-clip stage provides safety against hard clipping.

## Envelope snappiness

In each patch (and in `Voice::init()` defaults):

Short percussion: A=1-3, D=50-100, S=0, R=30-80
Lush pads: A=500-1500, D=1000-2500, S=0.7-0.9, R=1500-3000

## Header dot colors

In `UI.cpp`:

```cpp
static constexpr uint16_t DOT_HELD_COLOR      = ILI9341_GREEN;
static constexpr uint16_t DOT_RELEASING_COLOR = ILI9341_YELLOW;
static constexpr uint16_t DOT_OFF_COLOR       = 0x2104;
```

## Tab color palette

In `UI.cpp`:

```cpp
static constexpr uint16_t TAB_ACTIVE_BG   = 0x10B5;
static constexpr uint16_t TAB_ACTIVE_FG   = ILI9341_WHITE;
static constexpr uint16_t TAB_ACTIVE_EDGE = ILI9341_ORANGE;
static constexpr uint16_t TAB_INACTIVE_BG = 0x18E3;
static constexpr uint16_t TAB_INACTIVE_FG = 0x9CD3;
```

Swap values for other palettes (amber, terminal green, high-contrast) -- see comments in the file.

## Master output

masterMix is set to 0.25f per voice sub-mixer. That's headroom for 6 voices. Adjust in SynthEngine::begin():

```cpp
for (int i = 0; i < 4; i++) {
    subMix1.gain(i, 0.25f);  // ← here
    ...
}
```

## SGTL5000 output level

The codec's headphone amplifier level:

```cpp
codec.volume(0.6f);  // 0..1
```
