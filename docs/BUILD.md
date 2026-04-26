# Build & Setup

## Prerequisites

- [Visual Studio Code](https://code.visualstudio.com/)
- [PlatformIO IDE extension](https://platformio.org/install/ide?install=vscode)
- [Teensy Loader (CLI or GUI)](https://www.pjrc.com/teensy/loader.html)
- A microSD card (any capacity), formatted FAT32

## Cloning

```bash
git clone https://github.com/<your-user>/Juno-106.git
cd Juno-106
```

platformio.ini

The project ships with a known-good configuration pinned to a compatible Teensy platform and core. The Audio library is taken from the core, not from a separate GitHub download — this avoids a common version-skew issue.

```ini
[env:teensy41]
platform = teensy@^5.0.0
board = teensy41
framework = arduino

board_build.f_cpu = 600000000L

build_flags =
    -D USB_MIDI_SERIAL
    -D TEENSY_OPT_FASTER

lib_deps =
    PaulStoffregen/ILI9341_t3
    PaulStoffregen/XPT2046_Touchscreen

lib_ldf_mode = deep+
monitor_speed = 115200
```

Configuring USB Type

In PlatformIO's Teensy build, USB type is controlled by the build flag -D USB_MIDI_SERIAL. This enables both a USB serial console (for debug) and USB MIDI. If you want MIDI-only, change to -D USB_MIDI.

Building
```bash
pio run
```

Should complete without warnings or errors on a clean repo.

Uploading

With the Teensy plugged in:

```bash
pio run -t upload
````

Or via the PlatformIO sidebar: PROJECT TASKS → teensy41 → Upload.

First Boot

On first power-up with a fresh SD card:

1. The touch calibration wizard appears (3 corner crosshairs). Tap each accurately. Values are saved to /patches/touchcal.bin.
2. The factory patch installer writes 10 starter patches to slots 0–9 (controlled by INSTALL_FACTORY_ON_BOOT in Config.h).
3. The synth loads slot 0 and is ready to play.

After First Successful Boot

Edit src/Config.h and change:

```cpp
#define INSTALL_FACTORY_ON_BOOT 1
````

to

```cpp
#define INSTALL_FACTORY_ON_BOOT 0
```

Then reflash. This prevents your edited patches from being overwritten
every boot.

Debug Output

Open the serial monitor at 115200 baud:

```bash
pio device monitor
```

You'll see:

```
Installed 10 factory patches.
Juno-106 emulator ready.
```

plus any SD/calibration errors if something is wrong.

Clean Build

If you ever see mysterious link errors or library version weirdness:

```bash
rm -rf .pio
pio run
```

This wipes cached libraries and forces a fresh dependency resolution.
