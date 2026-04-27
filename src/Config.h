#pragma once
#include <Arduino.h>

// ---------- Polyphony ----------
#define MAX_VOICES        8
#define MAX_VOICES_LIMIT  16

// ---------- Display (ILI9341) ----------
#define TFT_CS    40
#define TFT_DC     9
#define TFT_RST  255

// ---------- Touch (XPT2046) ----------
#define TOUCH_CS   41
#define TOUCH_IRQ   2

// ---------- Audio Shield SD card ----------
#define SDCARD_CS_PIN    10
#define SDCARD_MOSI_PIN  7
#define SDCARD_SCK_PIN  14

// ---------- MIDI ----------
#define MIDI_SERIAL Serial6
#define MIDI_BAUD   31250

// ---------- Display & Touch orientation ----------
#define DISPLAY_ROTATION 3
#define TOUCH_ROTATION   3

#define SCREEN_W 320
#define SCREEN_H 240

// ---------- Touch calibration ----------
#define TOUCH_RAW_XMIN 300
#define TOUCH_RAW_XMAX 3800
#define TOUCH_RAW_YMIN 300
#define TOUCH_RAW_YMAX 3800
#define FORCE_TOUCH_RECAL    false
#define RECAL_ON_BOOT_TOUCH  true

// ---------- Control rate ----------
#define CONTROL_RATE_HZ 1000

// ---------- Patch storage ----------
#define NUM_PATCH_SLOTS 32

// ---------- Factory patches ----------
// Set to 1 to write factory patches to SD on next boot; then flip to 0.
#define INSTALL_FACTORY_ON_BOOT 1
