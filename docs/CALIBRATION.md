cat > docs/CALIBRATION.md << 'DOCEOF'
# Touch Calibration

## Why calibration matters

The XPT2046 reports raw ADC values (0-4095 per axis) that don't directly map to
screen pixels. The mapping depends on:

- How the touch panel was laminated to the LCD (may swap X/Y)
- Display rotation
- Manufacturing variance

Calibration learns the mapping and stores it on SD.

## When calibration runs

Automatically if:

1. No `/patches/touchcal.bin` exists on SD, or
2. `FORCE_TOUCH_RECAL true` in `Config.h`, or
3. The user holds the screen during boot (`RECAL_ON_BOOT_TOUCH true`).

Manually: tap the CAL button in the header's top-right corner.

## The wizard

Three crosshairs appear sequentially at corners:

1. Top-left
2. Top-right
3. Bottom-left

For each: touch directly on the crosshair center. The system:

- Waits for a touch
- Averages 16 raw readings
- Waits for release
- Moves to the next corner

After the third tap:

- Auto-detects X/Y swap (by checking which raw axis varied most TL->TR)
- Auto-detects per-axis inversion
- Extrapolates the 20-pixel inset to the true screen edges
- Saves to `/patches/touchcal.bin`
- Enters a brief verify mode (tap anywhere to see the mapped position)

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
Magic-number-checked on load.

## Manual adjustment

Fallback defaults in `Config.h`:

```cpp
#define TOUCH_RAW_XMIN 300
#define TOUCH_RAW_XMAX 3800
#define TOUCH_RAW_YMIN 300
#define TOUCH_RAW_YMAX 3800
```

These are only used if no calibration file exists.

## After changing display rotation

If you change `DISPLAY_ROTATION`, re-run the calibration wizard (the raw->pixel mapping changes).

## Troubleshooting

### Offset calibration
Re-run the wizard. Tap precisely on crosshair centers.

### Mirrored touch
Re-run the wizard -- it auto-detects mirroring.

### Calibration keeps running every boot
- Check SD card present and writable
- Verify `/patches/` directory was created
- Check serial for "SD save failed"

### No touch response
- Verify T_IRQ on pin 2 and T_CS on pin 41
- Check MOSI/MISO/SCK are on pins 11/12/13
- Try `FORCE_TOUCH_RECAL true` to force the wizard and observe its behavior
