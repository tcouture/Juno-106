# Roadmap

## Near-term (small, high-value)

- [ ] **MIDI channel selection** — per-patch or global setting for MIDI RX channel
- [ ] **MIDI clock sync** — arpeggiator locks to incoming MIDI clock
- [ ] **MIDI Learn** — long-press a slider, wiggle a CC, bind persistently to SD
- [ ] **Sustain pedal** (CC 64) handling
- [ ] **Expanded factory patch set** — grow from 10 to 20+ presets

## Medium-term (meaningful effort)

- [ ] **Stereo VU meter** in the header using `AudioAnalyzePeak`
- [ ] **Hardware encoders** (quadrature) for primary parameters (cutoff, resonance, env amount)
- [ ] **Hardware buttons** for tab navigation and patch up/down
- [ ] **Multi-patch morph** — interpolate between two patches via a wheel or CC
- [ ] **Chorus bypass crossfade** — avoid click when enabling/disabling

## Long-term (big features)

- [ ] **Sequencer page** — step sequencer with 16-note patterns per patch
- [ ] **SysEx dump / load** — MIDI-based patch exchange with DAWs
- [ ] **Wavetable oscillator option** — expand beyond basic Juno waveforms
- [ ] **Effects page** — add delay and reverb sends
- [ ] **Stereo voice panning** — position each note in the stereo field
- [ ] **Polyphonic aftertouch** — per-voice filter / amp modulation

## Audio-quality enhancements

- [ ] **Band-limited oscillators** — eliminate aliasing at high notes
- [ ] **Soft saturation / drive** on master output
- [ ] **Better filter models** — Moor-ladder emulation, diode-ladder option
- [ ] **True analog-style drift** — tiny per-voice detuning for organic feel

## UX polish

- [ ] **Patch browser with preview** — hold tap to audition before LOAD
- [ ] **Undo/redo** on parameter changes
- [ ] **Lock overlay** to prevent accidental touches during performance
- [ ] **Brightness control** for the TFT backlight (if PWM-capable)

## Community / open-source

- [ ] **Photos and demo video** in README
- [ ] **3D-printable case design**
- [ ] **PCB design** for a finished unit
- [ ] **LICENSE file** (MIT recommended)
- [ ] **CONTRIBUTING.md** guide

---

If you want to tackle one of these, the relevant section of [`ARCHITECTURE.md`](ARCHITECTURE.md) is the right starting point.