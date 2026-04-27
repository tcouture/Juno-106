# Build & Setup

## Prerequisites

- Visual Studio Code
- PlatformIO IDE extension
- Teensy Loader (CLI or GUI)
- A microSD card (any capacity), formatted FAT32

## Cloning
```bash
git clone https://github.com/<your-user>/Juno-106.git
cd Juno-106
```

platformio.ini

The project ships with a known-good configuration pinned to a compatible
Teensy platform and core:

```ini
[env:teensy41]
platform = teensy
board = teensy41
framework = arduino

board_build.f_cpu = 600000000L
board_build.usbtype = USB_MIDI_SERIAL

build_flags =
    -D USB_MIDI_SERIAL
    -D TEENSY_OPT_FASTER

lib_deps =
    PaulStoffregen/ILI9341_t3
    PaulStoffregen/XPT2046_Touchscreen

lib_ldf_mode = deep+
monitor_speed = 115200
```

Key points:

- `board_build.usbtype = USB_MIDI_SERIAL` enables both USB serial (for debug console) and USB MIDI device mode.
- USB host support uses `USBHost_t36`, which ships with the Teensy core — no separate dependency needed.
- Audio library likewise ships with the Teensy core and should NOT be added to `lib_deps` (avoids version skew).

## Building

```bash
pio run
```

Should complete cleanly. Warnings about `dspinst.h` internals (from the Audio library) are harmless.

## Uploading

With the Teensy plugged in:

```bash
pio run -t upload
````

Or via the PlatformIO sidebar: PROJECT TASKS -> teensy41 -> Upload.

## First Boot

On first power-up with a fresh SD card:

1. The touch calibration wizard appears (3 corner crosshairs).
   Tap each precisely. Values are saved to `/patches/touchcal.bin`.
2. The factory patch installer writes 22 starter patches to slots 0-21
   (controlled by `INSTALL_FACTORY_ON_BOOT` in `Config.h`).
3. The synth loads slot 0 and is ready to play.

## After First Successful Boot

Edit `src/Config.h`:

```cpp
#define INSTALL_FACTORY_ON_BOOT 1
````

to

```cpp
#define INSTALL_FACTORY_ON_BOOT 0
```

Then reflash. This prevents factory patches from overwriting your edits every boot.

## Debug Output

Open the serial monitor at 115200 baud:

```bash
pio device monitor
```

Expected log:

```
Installed 22 factory patches.
USB Host MIDI started. Plug in a keyboard.
Juno-106 emulator ready.
```

SD failures, USB host status, and calibration events also print here.

## Clean Build

If you ever see mysterious link errors or library version weirdness:

```bash
rm -rf .pio
pio run
```

This wipes cached libraries and forces a fresh dependency resolution.
