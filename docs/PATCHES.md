# Patches

## Storage format

Each patch is a binary dump of the `PatchData` struct, written to:

/patches/pNNN.bin

where `NNN` is the 3-digit, zero-padded slot number (000 through 031).

### PatchData struct fields

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

~88 bytes per patch. 32 slots = ~2.8 kB on SD.

## Slot grid

Displayed on the PATCH page as a 2x16 grid. Visual states:

- **Orange fill**: currently selected slot
- **Cyan border**: currently loaded slot (live patch came from here)
- **Navy fill**: occupied
- **Dark grey fill**: empty

The selected slot and the loaded slot are independent -- this lets you audition naming or properties of other slots without changing what's playing.

## Actions

- **LOAD**: reads the selected slot into the live patch.
- **SAVE**: prompts for a name (keyboard) then writes the live patch to the selected slot.
- **RENAME**: prompts for a new name for the selected slot's stored patch.
- **INIT**: resets the live patch to defaults. Does NOT write to SD.

## Factory patches (22 presets)

| Slot | Name | Character |
|------|------|-----------|
| 0  | BRASS STAB   | Bright filter stab, velocity -> cutoff |
| 1  | WARM PAD     | Slow attack, chorus II, LFO -> filter |
| 2  | PLUCKY BASS  | Heavy sub, snappy envelope, no chorus |
| 3  | SYNC LEAD    | Narrow PW, pitch LFO, mild glide |
| 4  | STRINGS      | Slow swell, chorus II |
| 5  | ORGAN        | Flat envelopes, sub heavy, Chorus I |
| 6  | FAT SAWS     | Thick PWM, slight detune LFO |
| 7  | BELL TINES   | Narrow PW, long release, no sustain |
| 8  | PERCUSSIVE   | Very short env, velocity -> cutoff |
| 9  | DREAMY LFO   | Slow filter LFO, chorus II |
| 10 | SUPERSAW     | Saw + pulse stacked, chorus II, subtle pitch LFO |
| 11 | WOOD KEYS    | Narrow PW, high resonance, velocity-sensitive |
| 12 | SUB BASS     | Pure sub, round bottom-end |
| 13 | RESO SWEEP   | High Q, slow filter envelope open |
| 14 | GLASS PAD    | Shimmery, chorus II, sine filter LFO |
| 15 | LFO WOBBLE   | Fast filter LFO (dubstep territory) |
| 16 | DETUNE LEAD  | Pitch LFO, mild glide, velocity -> cutoff |
| 17 | CHIME        | Bell-like, long release, bright |
| 18 | FAT ARP      | Dense mix, short envelope for arp work |
| 19 | SOFT FLUTE   | Pure pulse, vibrato, gentle |
| 20 | DARK PAD     | Low filter, slow LFO, chorus II |
| 21 | ACID         | High Q, snappy env, glide (classic 303-ish) |

Slots 22-31 are empty -- overwrite freely.

## Rolling your own factory patches

Edit `src/FactoryPatches.cpp`. Each block sets fields of a `PatchData` struct and calls `patchManager.savePatch(slot, p)`. The `makePatch(p, "NAME")` helper resets to defaults first.

During development, set `INSTALL_FACTORY_ON_BOOT 1` to rewrite patches every boot. Flip to `0` once the patch set is finalized to protect subsequent user edits.

## Backing up patches

Eject the SD card from the audio adapter, mount on a PC, copy `/patches/` somewhere safe. Patches are plain binary files.
