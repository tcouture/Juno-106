# Touch Calibration

## Why calibration matters

The XPT2046 touch controller reports raw ADC values (0–4095 per axis) that
don't directly map to screen pixels. The mapping depends on:

- How the touch panel was laminated to the LCD (may swap X/Y)
- Display rotation
- Manufacturing variance

Calibration learns the mapping and stores it on SD.

## When calibration runs

The wizard runs automatically if:

1. No `/patches/touchcal.bin` exists on SD, or
2. `FORCE_TOUCH_RECAL` in `Config.h` is `true`, or
3. The user is holding the screen during boot (`RECAL_ON_BOOT_TOUCH true`).

You can also trigger it manually anytime by tapping the **CAL** button in the top-right of the screen.

## The wizard

Three crosshairs appear sequentially at the corners:

1. Top-left
2. Top-right
3. Bottom-left

For each: touch directly on the center of the crosshair, as precisely as you can. The system:

- Waits for a clean touch
- Averages 16 consecutive raw readings
- Waits for you to lift
- Moves to the next corner

After the third tap, it:

- Computes whether axes need swapping (by checking which raw axis varied most on the TL→TR move)
- Computes whether each axis needs inverting
- Extrapolates the 20-pixel inset out to the true screen edges
- Writes the calibration to `/patches/touchcal.bin`
- Shows a brief verify mode where you can tap anywhere to see the mapped position

## Storage format

```cpp
struct TouchCalData {
    uint32_t magic;     // 0xCA11B8A7
    int16_t  xMin, xMax, yMin, yMax;
    uint8_t  swapXY;
    uint8_t  invertX;
    uint8_t  invertY;
};
```
On load, the magic number is checked; mismatched files are ignored.

## Manual adjustment

If auto-calibration isn't accurate, you can edit the defaults in Config.h:

```cpp
#define TOUCH_RAW_XMIN 300
#define TOUCH_RAW_XMAX 3800
#define TOUCH_RAW_YMIN 300
#define TOUCH_RAW_YMAX 3800
```

These are only used as fallback; a calibration file overrides them.

## Troubleshooting

Calibration seems "offset"
- Re-run the wizard. Tap as precisely as possible on the crosshair centers.

Touch is mirrored (left/right or up/down swapped)
- Re-run the wizard. It auto-detects these cases.

Calibration keeps running every boot
- Check SD card is present and writable.
- Confirm /patches/ directory is created (should be automatic).
- Check serial monitor for "SD save failed" after the wizard.

No touch response at all
- Verify T_IRQ (pin 2) wiring.
- Verify T_CS (pin 41) wiring.
- Check that MOSI/MISO/SCK are shared with the display on pins 11/12/13.
- Try setting FORCE_TOUCH_RECAL true to force the wizard and observe its behavior.

