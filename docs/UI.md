# User Interface

## Layout

```

┌───────────────────────────────────────────────────────────┐
│ JUNO-106  <patch name>                         [ CAL ]    │ 30 px  HEADER
├───────────────────────────────────────────────────────────┤
│  OSC   VCF   ENV   CHO   PRF   PAT                        │ 24 px  TABS
├───────────────────────────────────────────────────────────┤
│                                                           │
│                       (page body)                         │ 186 px BODY
│                                                           │
└───────────────────────────────────────────────────────────┘
```

Screen: 320×240 landscape.

## Tabs

| Tab | Page | Role |
|-----|------|------|
| OSC | Oscillators & LFO | Waveform mix, PW, LFO rate/depth, LFO destination, glide |
| VCF | Filter | HPF, cutoff, resonance, env amount |
| ENV | Envelopes & velocity | Amp ADSR, Filter ADSR, velocity destination + amount |
| CHO | Chorus | Mode (OFF/I/II) + rate + depth |
| PRF | Performance | Arpeggiator mode, rate, octaves |
| PAT | Patches | 32-slot grid + LOAD / SAVE / RENAME / INIT |

## Sliders

Sliders are vertical with a filled orange bar showing the current value. Touching anywhere on the slider sets the value relative to where you touched. Logarithmic sliders (e.g. cutoff, rate) are internally mapped with a log scale so small values at the low end get fine resolution.

## Modal dialogs

- **CAL confirm**: appears when the CAL header button is tapped. Touching outside the YES/NO area cancels.
- **On-screen keyboard**: appears for SAVE and RENAME on the PATCH page. Supports lowercase, uppercase (via SHIFT), digits, space, backspace, OK/CANCEL.
- **Status toast**: a 700 ms centered message after major actions (LOADED, SAVED, RENAMED, INIT PATCH, EMPTY SLOT).

## Touch handling

- Touch is polled at ~60 Hz (`16 ms` minimum interval) to avoid CPU saturation.
- Raw XPT2046 coordinates are passed through `TouchCalibration::mapToScreen()` which handles axis swap, invert, and min/max scaling.
- Sliders use a fast-path: `synth.setParam(ParamId, value)` instead of full `applyPatch()`, so dragging is smooth.

## Drawing strategy

- Only the tab's own page body is redrawn on page switch.
- Sliders redraw individually on touch — the rest of the page is untouched.
- The header is refreshed every 500 ms to keep the patch name current after renames.

## The CAL button

Located in the header's top-right corner. Tapping it opens a confirmation dialog. Confirming runs the full 3-corner calibration wizard. See [`CALIBRATION.md`](CALIBRATION.md).

## Page-specific controls

### OSC
- 7 sliders: SAW, PUL, SUB, PW, LFO-R, LFO-D, GLIDE
- LFO destination buttons on the right: OFF / PITCH / PW / FILT

### VCF
- 4 sliders: HPF, CUT, RES, ENV (env amount)

### ENV
- 8 sliders: A-A, A-D, A-S, A-R (amp env) + F-A, F-D, F-S, F-R (filter env)
- 1 slider: V-AMT (velocity amount)
- VEL DEST buttons at the bottom: OFF / VCA / CUT / LFO

### CHORUS
- 3 mode buttons: OFF / CHORUS I / CHORUS II
- 2 sliders: RATE, DEPTH (active when mode ≠ OFF)

### PERF (Arpeggiator)
- 5 mode buttons: OFF / UP / DN / UD / RND
- RATE and OCT with - / + stepper buttons

### PATCH
- 32-slot grid (populated slots show patch name; empty slots show "(empty)")
- Action buttons on the right: LOAD / SAVE / RENAME / INIT
- Tapping a slot selects it (highlighted in orange); the action buttons operate on the selected slot.
