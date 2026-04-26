# Troubleshooting

## Build fails

### `'F_CPU_ACTUAL' was not declared`
You have a version mismatch between the PlatformIO Teensy core and the Audio library. Fix by using the exact `platformio.ini` from `BUILD.md` and running:
```bash
rm -rf .pio
pio run
```

'boolean' does not name a type

Same root cause — stale Audio library. Same fix.

'AudioMixer4' is abstract

Same root cause.

'class XPT2046_Touchscreen' has no member named 'getPointRaw'

Older library version. The code calls getPoint() now, but if you see this, verify your headers are up to date.

'Arpeggiator' does not name a type

The Arpeggiator.h file is missing or the wrong content. Re-create from docs.

Runtime issues

No audio output

    Check audio adapter is seated properly.
    Check speakers/headphones are on the adapter, not a line out.
    Verify codec.enable() is called (inside synth.begin()).
    Try codec.volume(1.0f) temporarily.


MIDI not playing notes

    Connect a USB controller and confirm it enumerates as a MIDI device.
    Try MIDI_CHANNEL_OMNI (already default) — rules out channel mismatch.
    Check onNoteOn is being called (add Serial.println(note) for debug).
    For DIN MIDI: verify the opto-isolator circuit, especially 220Ω resistors and diode polarity.


Notes stuck on

- Send CC 123 (All Notes Off) or CC 120 (All Sound Off) from your controller. If persistent, check arpeggiator — if ARP is ON with no held notes, ensure allOff() is called when held drops to 0.

Pops / clicks on note changes

This should be rare with smoothing enabled. If you hear it:

- Verify synth.update() is called every loop iteration.
- Check AudioMemory(200) — increase if blocks are running out.
- Raise smoothing coefficients slightly (closer to 1.0 = slower, smoother).

Zipper noise on sweeps

- Lower the smoothing coefficients (closer to 0.9 = faster response but slight zipper). The default 0.94 is a balanced compromise.

Touch doesn't respond

- Run the calibration wizard (tap CAL or set FORCE_TOUCH_RECAL true).
- Verify T_IRQ (pin 2) and T_CS (pin 41) are wired correctly.
- Check serial output for "SD (audio shield) init failed" — without SD, calibration is lost every boot but touch should still basically work.

Touch positions are wrong

- Re-run calibration. If still wrong after tapping precisely, check that TOUCH_ROTATION in Config.h matches DISPLAY_ROTATION.

Display shows nothing

- Verify DC (pin 9), CS (pin 40), RST (tied to 3.3 V via 10 kΩ).
- Confirm MOSI/MISO/SCK on pins 11/12/13.
- Try lowering SPI speed if your wiring is long/unshielded (default is aggressive).

SD card errors

- Must be in the audio adapter SD slot, not the Teensy onboard slot.
- Must be FAT32 formatted.
- Try a different/smaller SD card — some older Teensy Audio library versions had issues with large/exotic cards.

Factory patches not loading

- Check INSTALL_FACTORY_ON_BOOT is 1 in Config.h for first run.
- After first successful run, change to 0 and reflash.
- Check serial for "Installed N factory patches."

Performance issues

Audio drops out or glitches

- Is AudioMemory() high enough for your voice count?
- Are any delay() calls >10 ms running in the main loop (e.g. inside showStatus)?
- Check AudioProcessorUsage() — should be well under 50%.

UI feels sluggish

- Touch is throttled to ~60 Hz; lower values in UI::update() for snappier feel.
- Avoid redrawing the full screen; use page-local redraw functions.

Frequent voice stealing at low poly counts

- Increase MAX_VOICES (and AudioMemory).
- Check that noteOff is being received — sometimes stuck notes masquerade as steal storms.

