# User Interface

## Layout

```
+-----------------------------------------------------------+
| CPU 14% ooo  BRASS STAB  | | U D H   [CAL]                | 26 px HEADER
|         ooo                                               |
+-----------------------------------------------------------+
| PATCH  OSC  VCF  ENV  CHORUS  PERF                        | 20 px TABS
+-----------------------------------------------------------+
|                                                           |
|                       (page body)                         | 194 px BODY
|                                                           |
+-----------------------------------------------------------+
```

Screen: 320x240 landscape, rotated 180 degrees via `DISPLAY_ROTATION 3`.

## Header status bar

Five distinct zones:

| Zone | Shows |
|------|-------|
| CPU % | `CPU NN%`, updated at 10 Hz from `AudioProcessorUsage()` |
| Voice dots | 2-row grid, auto-sized for voice count. Green = held, yellow = releasing, dark grey = idle |
| Patch name | Centered between voice dots and meter |
| Stereo peak meter | Two thin vertical bars, green/yellow/red zones, peak-hold ticks |
| MIDI activity | 3 dots labeled U/D/H -- flash their source color on incoming events |
| CAL button | Opens touch calibration confirm dialog |

MIDI indicator colors:
- **U** (green) = USB device (Teensy -> computer)
- **D** (magenta) = DIN Serial6
- **H** (cyan) = USB host (keyboard -> Teensy)

## Tabs

| Tab | Role |
|-----|------|
| PATCH | 32-slot grid + LOAD / SAVE / RENAME / INIT |
| OSC | Oscillator mix, PW, LFO, glide |
| VCF | HPF, cutoff, resonance, env amount, drive |
| ENV | Amp ADSR + Filter ADSR + velocity routing |
| CHORUS | Mode (OFF/I/II) + rate + depth |
| PERF | Arpeggiator + MIDI channel |

Active tab has a deep-blue fill with orange accent underline. Inactive tabs are charcoal.

## OSC page

- 7 sliders: SAW, PUL, SUB, PW, LFO-R, LFO-D, GLIDE
- LFO DEST strip at bottom-right: OFF / PIT / PW / FIL

## VCF page

- 5 sliders: HPF, CUT, RES, ENV, DRIVE

## ENV page

- 8 sliders: A-A, A-D, A-S, A-R (amp ADSR) + F-A, F-D, F-S, F-R (filter ADSR) + 1 V-AMT
- Section labels: "AMP ENV", "FILTER ENV", "VEL"
- VEL DEST strip at bottom-right: OFF / VCA / CUT / LFO

## CHORUS page

- Mode buttons: OFF / CHORUS I / CHORUS II
- 2 sliders: RATE, DEPTH (active when mode != OFF)

## PERF page

Two clearly separated sections:

### Arpeggiator
- 5 mode buttons: OFF / UP / DN / UD / RND
- RATE row with -/+ steppers (0.5-16 Hz)
- OCT row with -/+ steppers (1-4 octaves)

### MIDI (below a divider line)
- CH row with -/+ steppers (0 = ALL, 1-16 = specific channel)

## PATCH page

- 2-column x 16-row slot grid, single-line "NN NAME" layout
- Orange fill = currently selected slot
- Cyan border = currently loaded slot
- Dim grey fill = empty slot
- Action buttons on right: LOAD / SAVE / RENAME / INIT
- SAVE and RENAME open the on-screen keyboard for naming

## Modal dialogs

- **CAL confirm**: YES/NO dialog before running the calibration wizard
- **On-screen keyboard**: for SAVE/RENAME, with shift, digits, symbols, backspace, OK/CANCEL
- **Status toast**: 700 ms centered message after LOAD/SAVE/RENAME/INIT

## Touch handling

- Polled at ~60 Hz (16 ms minimum interval)
- Raw XPT2046 coordinates run through `TouchCalibration::mapToScreen()` (handles axis swap, invert, scaling)
- Sliders use `synth.setParam()` fast path -- no full `applyPatch()` per touch event

## Drawing strategy

- Full screen only redrawn on page switch
- Sliders redraw individually on touch
- Header CPU / voice dots / meter / MIDI activity refresh every 100 ms
- Full header redraw only triggered when the patch name changes
