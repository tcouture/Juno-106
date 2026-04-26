# Patches

## Storage format

Each patch is a binary dump of the `PatchData` struct, written to:

/patches/pNNN.bin

where `NNN` is the 3-digit, zero-padded slot number (000 through 031).

### `PatchData` struct layout

Defined in `SynthEngine.h`:

```cpp
struct PatchData {
    char  name[17];        // null-terminated, up to 16 visible chars
    float sawLevel, pulseLevel, subLevel, pulseWidth;
    float cutoff, resonance, envAmount, hpfCutoff;
    float ampA, ampD, ampS, ampR;
    float fltA, fltD, fltS, fltR;
    float lfoRate, lfoDepth;
    uint8_t lfoDest, lfoShape;
    uint8_t chorusMode;
    float chorusRate, chorusDepth;
    uint8_t velDest;
    float velAmount;
    float glideMs;
};
````

~80 bytes per patch. Total space for 32 slots: ~2.5 kB. Trivial for any SD card.

### Slot grid

The PATCH page displays all 32 slots. Slot color:

    Orange: currently selected
    Navy: occupied
    Dark grey: empty


### Actions

    LOAD: reads the selected slot into the live patch.
    SAVE: prompts for a name (keyboard) then writes the live patch to the selected slot.
    RENAME: prompts for a new name for the selected slot's stored patch (and updates the live patch name if it was the currently-loaded slot).
    INIT: resets the live patch to defaults. Does not write to SD; you must SAVE after if you want to persist.

## Factory patches

Ten starter patches are installed on first boot (controlled by INSTALL_FACTORY_ON_BOOT in Config.h):

| Slot | Name | Character |
|:-----|:-----|:----------|
|0 | BRASS STAB |	Short, bright, filter env + velocity to cutoff |
| 1 |	WARM PAD |	Slow attack, chorus II, gentle LFO-filter |
| 2 |	PLUCKY BASS |	Short release, heavy sub, no chorus |
| 3 |	SYNC LEAD |	Narrow PW, pitch LFO, mild glide |
| 4 |	STRINGS |	Slow swell, chorus II, big sound |
| 5 |	ORGAN |	Flat envelopes, sub heavy, Chorus I |
| 6 |	FAT SAWS |	Thick PWM, slight detune LFO |
| 7 |	BELL TINES |	Narrow PW, long release, no sustain |
| 8 |	PERCUSSIVE |	Very short env, velocity to cutoff, punchy |
| 9 |	DREAMY LFO |	Slow filter LFO, chorus II, light glide |

Slots 10–31 are empty. Overwrite freely.

Rolling your own factory patches

Edit src/FactoryPatches.cpp. Each block sets fields of a PatchData struct and calls patchManager.savePatch(slot, p). The makePatch(p, "NAME") helper resets to defaults before your tweaks, so you only need to specify what differs.

Tips:

    Use INSTALL_FACTORY_ON_BOOT 1 during development to rewrite patches each boot.
    Set back to 0 for shipped/deployed code.

## Backing up patches

Eject the SD card from the audio adapter. Mount it on a PC. Copy the /patches/ folder somewhere safe. Patches are just binary files — no special tooling required.

## Troubleshooting empty grid on first boot

If you're seeing all "(empty)" slots after first boot:

- Check SD card is in the audio adapter slot, not the Teensy onboard slot.
- Verify SD is FAT32 formatted.
- Check serial monitor for "SD (audio shield) init failed".
- Confirm INSTALL_FACTORY_ON_BOOT is 1 in Config.h.

