# Troubleshooting

## Build fails

### `'F_CPU_ACTUAL' was not declared`
You have a version mismatch between the PlatformIO Teensy core and the Audio library. Fix by using the exact `platformio.ini` from `BUILD.md` and running:
```bash
rm -rf .pio
pio run
```
### `'boolean' does not name a type` / `'AudioMixer4' is abstract`
Same root cause. Same fix.

### `'ParamId' has no member named 'Drive'`
You added `setParam(ParamId::Drive, ...)` or a DRIVE slider but didn't add `Drive` to the `ParamId` enum in `SynthEngine.h`. Add it.

### `'ch' was not declared in this scope`
A MIDI handler function is missing a named first parameter. Change `byte` to `byte ch` in handler signatures that use `synth.matchesChannel(ch)`.

### `'class XPT2046_Touchscreen' has no member named 'getPointRaw'`
Older library version. The code uses `getPoint()` -- verify headers are up to date.

## Runtime

### No audio output
- Check audio adapter is seated
- Check speakers/headphones are on the adapter, not line out
- Verify `codec.enable()` is called
- Try `codec.volume(1.0f)` temporarily

### No MIDI playing notes
- Verify at least one of the three sources (USB device / USB host / DIN) is connected.
- Check the MIDI activity indicators -- the corresponding dot should flash when events arrive.
- Verify the patch's MIDI channel matches what the controller is sending, or set channel to ALL (0).

### USB host not detecting keyboard
- Must be a class-compliant USB MIDI device.
- Power: bus-powered controllers need >=1 A supply to the Teensy.
- Try plugging the keyboard in AFTER the Teensy boots.
- Check serial for "USB Host MIDI started" message at boot.
- Add diagnostic `Serial.println(hostMIDI ? "YES" : "no")` in `UsbHostMidi::update()` to confirm detection.

### Stuck notes
- Send CC 123 (All Notes Off) from your controller.
- Check arpeggiator -- if ARP is ON with no held notes, `allOff()` should fire automatically.
- Test sustain pedal: if it's sending CC 64 = 127 but never 0, notes never release.

### Pops or clicks on note changes
- Verify `synth.update()` runs every `loop()` iteration.
- Increase `AudioMemory()` if blocks are running out.
- Raise smoothing coefficients slightly (closer to 1.0).

### Zipper noise on sweeps
- Lower smoothing coefficients (closer to 0.9).
- Default 0.94 is a good compromise.

### Touch doesn't respond
- Run calibration wizard (tap CAL, or `FORCE_TOUCH_RECAL true`).
- Check T_IRQ (pin 2), T_CS (pin 41) wiring.

### Touch positions are wrong
- Re-run calibration.
- Verify `TOUCH_ROTATION` matches `DISPLAY_ROTATION` in `Config.h`.

### Display blank
- Verify DC (pin 9), CS (pin 40), RST pull-up.
- MOSI/MISO/SCK on pins 11/12/13.

### SD card errors
- Must be in the audio adapter SD slot, not Teensy onboard.
- FAT32 formatted.
- Try a different/smaller card.

### Factory patches not loading
- `INSTALL_FACTORY_ON_BOOT 1` in `Config.h` for first run.
- After first run, set to `0` and reflash.
- Check serial for "Installed 22 factory patches."

## Performance

### Audio dropouts / glitches
- `AudioMemory()` high enough? (240 for 8 voices)
- Any `delay()` >10 ms in the main loop?
- Check `AudioProcessorUsage()` -- should be well under 50% even at full polyphony.

### UI sluggish
- Touch poll is 16 ms; lower for snappier feel, higher to save CPU.
- Avoid redrawing the full screen -- use page-local redraws.

### CPU meter high
- Band-limited oscillators are ~2x the CPU of plain ones.
- 8-voice chords with band-limited osc + chorus + drive typically run 15-25% on Teensy 4.1.
- If you see >60%, check whether something is redrawing the UI too aggressively.