#pragma once
#include <Arduino.h>
#include <ILI9341_t3.h>

// =============================================================================
// UI Theme — Ableton-Live-inspired dark palette
//
// Color tokens are RGB565. Comments show approximate sRGB values.
// =============================================================================

// ----- Surfaces -----
static constexpr uint16_t TH_BG_DEEPEST   = 0x0841;   // (8,8,8)    body
static constexpr uint16_t TH_BG_DARK      = 0x18C3;   // (24,24,24) header/footer
static constexpr uint16_t TH_BG_PANEL     = 0x2965;   // (40,44,40) cells/buttons
static constexpr uint16_t TH_BG_PANEL_HI  = 0x4208;   // (66,66,66) hover/highlight
static constexpr uint16_t TH_RULE         = 0x3186;   // (50,50,50) separators

// ----- Text -----
static constexpr uint16_t TH_TEXT_DIM     = 0x738E;   // secondary labels
static constexpr uint16_t TH_TEXT_NORM    = 0xB596;   // body text
static constexpr uint16_t TH_TEXT_HI      = 0xEF7D;   // active text, values

// ----- Accent (used sparingly!) -----
static constexpr uint16_t TH_ACCENT       = 0xFD20;   // orange
static constexpr uint16_t TH_ACCENT_DIM   = 0x8200;   // darker orange backdrops

// ----- State colors (kept distinct from accent) -----
static constexpr uint16_t TH_VOICE_HELD   = 0x07E0;   // green
static constexpr uint16_t TH_VOICE_REL    = 0xFFE0;   // yellow
static constexpr uint16_t TH_VOICE_OFF    = 0x2104;   // very dark grey

static constexpr uint16_t TH_METER_OK     = 0x07E0;   // green
static constexpr uint16_t TH_METER_LOUD   = 0xFFE0;   // yellow
static constexpr uint16_t TH_METER_CLIP   = 0xF800;   // red
static constexpr uint16_t TH_METER_HOLD   = 0xEF7D;   // hold tick
static constexpr uint16_t TH_METER_BG     = 0x18C3;   // bg

static constexpr uint16_t TH_MIDI_USBDEV  = 0x07E0;   // green
static constexpr uint16_t TH_MIDI_DIN     = 0xF81F;   // magenta
static constexpr uint16_t TH_MIDI_USBHOST = 0x07FF;   // cyan
static constexpr uint16_t TH_MIDI_OFF     = 0x2104;

// ----- Page status colors -----
static constexpr uint16_t TH_BTN_OK       = 0x05E0;   // green for confirmation
static constexpr uint16_t TH_BTN_WARN     = 0xC000;   // red for cautionary

// ----- Dimensions -----
static constexpr int TH_HEADER_H          = 24;
static constexpr int TH_TABS_H            = 18;
static constexpr int TH_RULE_H            = 1;        // separator thickness
static constexpr int TH_ACCENT_BAR_H      = 2;        // active tab marker
