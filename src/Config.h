#pragma once
#include <Arduino.h>

// ---------- Polyphony ----------
#define MAX_VOICES        6      // Start with 6, extensible to 16
#define MAX_VOICES_LIMIT  16

// ---------- Display (ILI9341) ----------
#define TFT_CS    40
#define TFT_DC     9
#define TFT_RST  255            // tied to 3.3V via 10K pullup
// MOSI=11, MISO=12, SCK=13 (main SPI)

// ---------- Touch (XPT2046) ----------
#define TOUCH_CS   41
#define TOUCH_IRQ   2

// ---------- Audio Shield SD card ----------
#define SDCARD_CS_PIN    10
#define SDCARD_MOSI_PIN  7
#define SDCARD_SCK_PIN  14

// ---------- MIDI ----------
#define MIDI_SERIAL Serial6    // pins 24(RX)/25(TX)
#define MIDI_BAUD   31250

// ---------- Display & Touch orientation ----------
// Rotation values: 0=portrait, 1=landscape, 2=portrait flipped, 3=landscape flipped
#define DISPLAY_ROTATION 1     // landscape
#define TOUCH_ROTATION   1     // landscape (adjust independently if axes are swapped/mirrored)

#define SCREEN_W 320
#define SCREEN_H 240

// ---------- Touch calibration (raw XPT2046 values) ----------
// Adjust these after running a calibration sketch if touch feels off.
#define TOUCH_RAW_XMIN 300
#define TOUCH_RAW_XMAX 3800
#define TOUCH_RAW_YMIN 300
#define TOUCH_RAW_YMAX 3800

// ---------- Control rate ----------
#define CONTROL_RATE_HZ 1000        // LFO + env->cutoff update rate

// ---------- Patch storage ----------
#define NUM_PATCH_SLOTS 32

// ---------- Touch calibration triggers ----------
#define FORCE_TOUCH_RECAL    false   // true: always run wizard on boot
#define RECAL_ON_BOOT_TOUCH  true    // true: holding screen at boot re-runs wizard
