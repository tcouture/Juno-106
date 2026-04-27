# Roadmap

## Near-term (small, high-value)

- [ ] **MIDI clock sync** -- arpeggiator locks to incoming MIDI clock
- [ ] **MIDI Learn** -- long-press a slider, wiggle a CC, bind persistently to SD
- [ ] **SysEx dump / load** -- MIDI-based patch exchange with DAWs
- [ ] **Polyphonic aftertouch** -- per-voice filter / amp modulation
- [ ] **Sustain pedal polarity setting** -- for inverted-polarity pedals

## Medium-term

- [ ] **Sequencer page** -- 16-step sequencer per patch
- [ ] **Multi-patch morph** -- interpolate between two patches via CC
- [ ] **Hardware encoders** for primary parameters (cutoff, resonance, env amount)
- [ ] **Hardware buttons** for tab navigation and patch recall
- [ ] **Chorus bypass crossfade** -- avoid click when enabling/disabling
- [ ] **Parameter lock / performance mode** -- prevent accidental touches during play

## Long-term

- [ ] **Per-oscillator waveform selection** (currently fixed saw+pulse+sub-square, Juno-authentic)
- [ ] **Effects page** -- delay and reverb sends
- [ ] **Stereo voice panning** -- position each note in the stereo field
- [ ] **Wavetable oscillator option** -- expand beyond the base Juno waveforms

## Audio-quality enhancements

- [ ] **Moog-ladder filter model** (or diode-ladder alternative)
- [ ] **Analog-style voice drift** -- tiny per-voice detuning for organic feel
- [ ] **Asymmetric soft clipper option** -- tube-amp character for drive

## UX polish

- [ ] **Patch browser with preview** -- hold tap to audition before LOAD
- [ ] **Undo/redo** on parameter changes
- [ ] **Lock overlay** to prevent accidental touches during performance
- [ ] **Brightness control** for the TFT backlight (if PWM-capable)

## Community / open-source

- [ ] **Photos and demo video** in README
- [ ] **3D-printable case design**
- [ ] **PCB design** for a finished unit
- [ ] **LICENSE file**
- [ ] **CONTRIBUTING.md** guide

---

If you want to tackle one of these, the relevant section of [`ARCHITECTURE.md`](ARCHITECTURE.md) is the right starting point.