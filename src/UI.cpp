#include "UI.h"
#include "Config.h"
#include "SynthEngine.h"
#include "PatchManager.h"
#include "TouchCalibration.h"
#include "Arpeggiator.h"
#include "OnScreenKeyboard.h"
#include "MidiActivity.h"
#include "UITheme.h"
#include <ILI9341_t3.h>
#include <XPT2046_Touchscreen.h>

static ILI9341_t3 tft(TFT_CS, TFT_DC, TFT_RST);
static XPT2046_Touchscreen ts(TOUCH_CS, TOUCH_IRQ);

UI ui;

// -----------------------------------------------------------------------------
// Slider description (vertical)
// -----------------------------------------------------------------------------
struct Slider {
    const char* label;
    int x, y, w, h;
    float* value;
    float  min, max;
    bool   logarithmic;
    ParamId paramId;
    const char* unit;     // "Hz", "ms", "%", "smp", or nullptr
};

static Slider pageSliders[12];
static int    pageSliderCount = 0;

// -----------------------------------------------------------------------------
// Inline (horizontal) slider description
// -----------------------------------------------------------------------------
struct InlineSlider {
    int x, y, w, h;
    float* value;
    float  min, max;
    bool   logarithmic;
    ParamId paramId;
    const char* label;
    const char* unit;
};

// -----------------------------------------------------------------------------
// Layout constants
// -----------------------------------------------------------------------------
static constexpr int HEADER_H  = TH_HEADER_H;
static constexpr int TABS_Y    = HEADER_H;
static constexpr int TABS_H    = TH_TABS_H;
static constexpr int BODY_Y    = HEADER_H + TABS_H;
static constexpr int BODY_H    = SCREEN_H - BODY_Y;
static constexpr int SLIDER_W  = 32;
static constexpr int SLIDER_H  = 110;
static constexpr int SLIDER_Y  = BODY_Y + 10;

static constexpr int ENV_LABEL_Y  = BODY_Y + 4;
static constexpr int ENV_SLIDER_Y = BODY_Y + 18;
static constexpr int ENV_SLIDER_H = SLIDER_H - 18;

static constexpr int DEST_BTN_W   = 44;
static constexpr int DEST_BTN_H   = 16;
static constexpr int DEST_BTN_GAP = 2;

static const char* pageNames[PAGE_COUNT] = { "PATCH", "OSC", "VCF", "ENV", "CHORUS", "PERF" };

// =============================================================================
// Drawing primitives
// =============================================================================

static void drawHRule(int y, uint16_t color = TH_RULE) {
    tft.drawFastHLine(0, y, SCREEN_W, color);
}

static void drawOutlinedButton(int x, int y, int w, int h, const char* label,
                               bool active = false, bool warn = false) {
    uint16_t edge = active ? TH_ACCENT : (warn ? TH_BTN_WARN : TH_TEXT_DIM);
    uint16_t txt  = active ? TH_TEXT_HI : (warn ? TH_TEXT_HI : TH_TEXT_NORM);

    if (active) {
        tft.fillRect(x + 1, y + 1, w - 2, h - 2, TH_BG_PANEL_HI);
    } else {
        tft.fillRect(x + 1, y + 1, w - 2, h - 2, TH_BG_DEEPEST);
    }
    tft.drawRect(x, y, w, h, edge);

    tft.setTextColor(txt);
    tft.setTextSize(1);
    int tw = strlen(label) * 6;
    tft.setCursor(x + (w - tw) / 2, y + (h - 8) / 2);
    tft.print(label);
}

static void formatValue(char* out, size_t n, float v, const Slider& s) {
    if (s.unit && strcmp(s.unit, "Hz") == 0) {
        if (v < 10.0f) snprintf(out, n, "%.2fHz", v);
        else           snprintf(out, n, "%.0fHz", v);
    } else if (s.unit && strcmp(s.unit, "ms") == 0) {
        if (v < 10.0f) snprintf(out, n, "%.1fms", v);
        else           snprintf(out, n, "%.0fms", v);
    } else if (s.unit && strcmp(s.unit, "%") == 0) {
        snprintf(out, n, "%.0f%%", v * 100.0f);
    } else if (s.unit && strcmp(s.unit, "smp") == 0) {
        snprintf(out, n, "%.0f", v);
    } else {
        if (s.max <= 2.0f)        snprintf(out, n, "%.2f", v);
        else if (s.max <= 10.0f)  snprintf(out, n, "%.1f", v);
        else                      snprintf(out, n, "%.0f", v);
    }
}

static void drawSlimSlider(const Slider& s) {
    tft.fillRect(s.x, s.y, s.w, s.h + 22, TH_BG_DEEPEST);

    int trackX = s.x + s.w / 2;
    int trackY = s.y;
    int trackH = s.h;

    tft.drawFastVLine(trackX, trackY, trackH, TH_BG_PANEL_HI);

    float v = *s.value;
    float norm;
    if (s.logarithmic && s.min > 0.0f && s.max > 0.0f) {
        norm = logf(v / s.min) / logf(s.max / s.min);
    } else {
        norm = (v - s.min) / (s.max - s.min);
    }
    if (norm < 0) norm = 0;
    if (norm > 1) norm = 1;

    int handleY = trackY + trackH - 1 - (int)((trackH - 1) * norm);

    if (handleY < trackY + trackH - 1) {
        tft.drawFastVLine(trackX, handleY, trackY + trackH - handleY, TH_TEXT_NORM);
    }

    tft.fillRect(trackX - 6, handleY - 1, 13, 3, TH_ACCENT);

    tft.setTextSize(1);
    tft.setTextColor(TH_TEXT_DIM);
    int tw = strlen(s.label) * 6;
    tft.setCursor(s.x + (s.w - tw) / 2, s.y + s.h + 3);
    tft.print(s.label);

    char vbuf[12];
    formatValue(vbuf, sizeof(vbuf), v, s);
    tft.setTextColor(TH_TEXT_NORM);
    int vw = strlen(vbuf) * 6;
    tft.setCursor(s.x + (s.w - vw) / 2, s.y + s.h + 13);
    tft.print(vbuf);
}

// -----------------------------------------------------------------------------
// Inline slider helpers
// -----------------------------------------------------------------------------
static void drawInlineSlider(const InlineSlider& s) {
    tft.fillRect(s.x, s.y, s.w, s.h, TH_BG_DEEPEST);

    tft.setTextSize(1);
    tft.setTextColor(TH_TEXT_DIM);
    int labelW = strlen(s.label) * 6;
    tft.setCursor(s.x, s.y + (s.h - 8) / 2);
    tft.print(s.label);

    int trackX0 = s.x + labelW + 4;
    int trackX1 = s.x + s.w - 28;     // reserve 28 px on right for value
    int trackW  = trackX1 - trackX0;
    int trackY  = s.y + s.h / 2;

    tft.drawFastHLine(trackX0, trackY, trackW, TH_BG_PANEL_HI);

    float v = *s.value;
    float norm;
    if (s.logarithmic && s.min > 0.0f && s.max > 0.0f) {
        norm = logf(v / s.min) / logf(s.max / s.min);
    } else {
        norm = (v - s.min) / (s.max - s.min);
    }
    if (norm < 0) norm = 0;
    if (norm > 1) norm = 1;

    int handleX = trackX0 + (int)(trackW * norm);

    if (handleX > trackX0) {
        tft.drawFastHLine(trackX0, trackY, handleX - trackX0, TH_TEXT_NORM);
    }
    tft.fillRect(handleX - 1, trackY - 2, 3, 5, TH_ACCENT);

    char vbuf[12];
    Slider tmp = { "", 0, 0, 0, 0, s.value, s.min, s.max, s.logarithmic, s.paramId, s.unit };
    formatValue(vbuf, sizeof(vbuf), v, tmp);
    tft.setTextColor(TH_TEXT_NORM);
    tft.setCursor(s.x + s.w - 26, s.y + (s.h - 8) / 2);
    tft.print(vbuf);
}

static void inlineSliderApply(const InlineSlider& s, int x) {
    int labelW = strlen(s.label) * 6;
    int trackX0 = s.x + labelW + 4;
    int trackX1 = s.x + s.w - 28;
    int trackW  = trackX1 - trackX0;
    if (x < trackX0) x = trackX0;
    if (x > trackX1) x = trackX1;
    float norm = (float)(x - trackX0) / (float)trackW;
    if (norm < 0) norm = 0;
    if (norm > 1) norm = 1;
    float v;
    if (s.logarithmic && s.min > 0.0f && s.max > 0.0f) {
        v = s.min * powf(s.max / s.min, norm);
    } else {
        v = s.min + norm * (s.max - s.min);
    }
    *s.value = v;
    synth.setParam(s.paramId, v);
}

// =============================================================================
// Page builders
// =============================================================================
static void buildOscSliders() {
    PatchData& p = synth.patch();
    int x = 6;
    int gap = SLIDER_W + 8;
    pageSliders[0] = { "SAW",   x, SLIDER_Y, SLIDER_W, SLIDER_H, &p.sawLevel,   0.0f, 1.0f, false, ParamId::SawLevel,   nullptr }; x += gap;
    pageSliders[1] = { "PUL",   x, SLIDER_Y, SLIDER_W, SLIDER_H, &p.pulseLevel, 0.0f, 1.0f, false, ParamId::PulseLevel, nullptr }; x += gap;
    pageSliders[2] = { "SUB",   x, SLIDER_Y, SLIDER_W, SLIDER_H, &p.subLevel,   0.0f, 1.0f, false, ParamId::SubLevel,   nullptr }; x += gap;
    pageSliders[3] = { "PW",    x, SLIDER_Y, SLIDER_W, SLIDER_H, &p.pulseWidth, 0.05f, 0.95f, false, ParamId::PulseWidth, nullptr }; x += gap;
    pageSliders[4] = { "RATE",  x, SLIDER_Y, SLIDER_W, SLIDER_H, &p.lfoRate,    0.05f, 20.0f, true, ParamId::LfoRate,   "Hz" }; x += gap;
    pageSliders[5] = { "DEPTH", x, SLIDER_Y, SLIDER_W, SLIDER_H, &p.lfoDepth,   0.0f, 1.0f, false, ParamId::LfoDepth,  nullptr }; x += gap;
    pageSliders[6] = { "GLIDE", x, SLIDER_Y, SLIDER_W, SLIDER_H, &p.glideMs,    0.0f, 1000.0f, false, ParamId::GlideMs, "ms" };
    pageSliderCount = 7;
}

static void buildVcfSliders() {
    PatchData& p = synth.patch();
    int x = 14;
    int gap = SLIDER_W + 18;
    pageSliders[0] = { "HPF",   x, SLIDER_Y, SLIDER_W, SLIDER_H, &p.hpfCutoff, 20.0f, 1000.0f, true, ParamId::HpfCutoff, "Hz" }; x += gap;
    pageSliders[1] = { "CUT",   x, SLIDER_Y, SLIDER_W, SLIDER_H, &p.cutoff,    40.0f, 8000.0f, true, ParamId::Cutoff,    "Hz" }; x += gap;
    pageSliders[2] = { "RES",   x, SLIDER_Y, SLIDER_W, SLIDER_H, &p.resonance, 0.7f,  5.0f,    false, ParamId::Resonance, nullptr }; x += gap;
    pageSliders[3] = { "ENV",   x, SLIDER_Y, SLIDER_W, SLIDER_H, &p.envAmount, 0.0f,  1.0f,    false, ParamId::EnvAmount, nullptr }; x += gap;
    pageSliders[4] = { "DRIVE", x, SLIDER_Y, SLIDER_W, SLIDER_H, &p.drive,     1.0f,  8.0f,    false, ParamId::Drive,     nullptr };
    pageSliderCount = 5;
}

static void buildEnvSliders() {
    PatchData& p = synth.patch();
    int x = 12;
    int gapIn  = 4;
    int gapOut = 22;
    pageSliders[0] = { "A", x, ENV_SLIDER_Y, SLIDER_W, ENV_SLIDER_H, &p.ampA, 0.0f, 3000.0f, true,  ParamId::AmpA, "ms" }; x += SLIDER_W + gapIn;
    pageSliders[1] = { "D", x, ENV_SLIDER_Y, SLIDER_W, ENV_SLIDER_H, &p.ampD, 0.0f, 3000.0f, true,  ParamId::AmpD, "ms" }; x += SLIDER_W + gapIn;
    pageSliders[2] = { "S", x, ENV_SLIDER_Y, SLIDER_W, ENV_SLIDER_H, &p.ampS, 0.0f, 1.0f,    false, ParamId::AmpS, nullptr }; x += SLIDER_W + gapIn;
    pageSliders[3] = { "R", x, ENV_SLIDER_Y, SLIDER_W, ENV_SLIDER_H, &p.ampR, 0.0f, 5000.0f, true,  ParamId::AmpR, "ms" }; x += SLIDER_W + gapOut;
    pageSliders[4] = { "A", x, ENV_SLIDER_Y, SLIDER_W, ENV_SLIDER_H, &p.fltA, 0.0f, 3000.0f, true,  ParamId::FltA, "ms" }; x += SLIDER_W + gapIn;
    pageSliders[5] = { "D", x, ENV_SLIDER_Y, SLIDER_W, ENV_SLIDER_H, &p.fltD, 0.0f, 3000.0f, true,  ParamId::FltD, "ms" }; x += SLIDER_W + gapIn;
    pageSliders[6] = { "S", x, ENV_SLIDER_Y, SLIDER_W, ENV_SLIDER_H, &p.fltS, 0.0f, 1.0f,    false, ParamId::FltS, nullptr }; x += SLIDER_W + gapIn;
    pageSliders[7] = { "R", x, ENV_SLIDER_Y, SLIDER_W, ENV_SLIDER_H, &p.fltR, 0.0f, 5000.0f, true,  ParamId::FltR, "ms" };
    pageSliderCount = 8;
}

static void buildChorusSliders() {
    PatchData& p = synth.patch();
    int x = 30;
    int y = BODY_Y + 60;
    int h = SLIDER_H - 20;
    pageSliders[0] = { "RATE",  x, y, SLIDER_W, h, &p.chorusRate,  0.05f, 8.0f, true, ParamId::ChorusRate,  "Hz" };  x += SLIDER_W + 18;
    pageSliders[1] = { "DEPTH", x, y, SLIDER_W, h, &p.chorusDepth, 0.0f, 80.0f, false, ParamId::ChorusDepth, "smp" };
    pageSliderCount = 2;
}

static void buildPerfSliders() { pageSliderCount = 0; }

static void buildPageSliders(UIPage page) {
    switch (page) {
        case PAGE_OSC:    buildOscSliders(); break;
        case PAGE_VCF:    buildVcfSliders(); break;
        case PAGE_ENV:    buildEnvSliders(); break;
        case PAGE_CHORUS: buildChorusSliders(); break;
        case PAGE_PERF:   buildPerfSliders(); break;
        default:          pageSliderCount = 0; break;
    }
}

// =============================================================================
// Bottom dest strip (LFO DEST / VEL DEST)
// =============================================================================
static void destStripLayout(int& bx, int& by, int count) {
    bx = SCREEN_W - count * DEST_BTN_W - (count - 1) * DEST_BTN_GAP - 6;
    by = SCREEN_H - DEST_BTN_H - 4;
}

static void drawDestStrip(const char* const* labels, int count, int current, const char* title) {
    int bx, by;
    destStripLayout(bx, by, count);

    tft.setTextSize(1);
    tft.setTextColor(TH_TEXT_DIM);
    tft.setCursor(bx, by - 11);
    tft.print(title);

    for (int i = 0; i < count; i++) {
        int xx = bx + i * (DEST_BTN_W + DEST_BTN_GAP);
        bool active = (current == i);

        if (active) tft.fillRect(xx, by, DEST_BTN_W, DEST_BTN_H, TH_BG_PANEL_HI);
        else        tft.fillRect(xx, by, DEST_BTN_W, DEST_BTN_H, TH_BG_DEEPEST);

        tft.setTextColor(active ? TH_TEXT_HI : TH_TEXT_DIM);
        int tw = strlen(labels[i]) * 6;
        tft.setCursor(xx + (DEST_BTN_W - tw) / 2, by + (DEST_BTN_H - 8) / 2);
        tft.print(labels[i]);

        if (active) tft.fillRect(xx, by + DEST_BTN_H, DEST_BTN_W, 1, TH_ACCENT);
    }
}

static int destStripHit(int x, int y, int count) {
    int bx, by;
    destStripLayout(bx, by, count);
    if (y < by || y > by + DEST_BTN_H) return -1;
    for (int i = 0; i < count; i++) {
        int xx = bx + i * (DEST_BTN_W + DEST_BTN_GAP);
        if (x >= xx && x <= xx + DEST_BTN_W) return i;
    }
    return -1;
}

// =============================================================================
// UI init
// =============================================================================
void UI::begin() {
    tft.begin();
    tft.setRotation(DISPLAY_ROTATION);
    tft.fillScreen(TH_BG_DEEPEST);

    ts.begin();
    ts.setRotation(0);

    touchCal.begin(&tft, &ts);
    bool needCal = FORCE_TOUCH_RECAL;
    if (!needCal && !touchCal.loadFromSD()) needCal = true;
    if (!needCal && RECAL_ON_BOOT_TOUCH && ts.touched()) needCal = true;
    if (needCal) touchCal.runWizard();

    calBtnW = 32; calBtnH = 14;
    calBtnX = SCREEN_W - calBtnW - 4;
    calBtnY = (HEADER_H - calBtnH) / 2;

    computeHeaderLayout();

    osKeyboard.begin(&tft, &ts);

    drawAll();
}

// =============================================================================
// Header
// =============================================================================
void UI::computeHeaderLayout() {
    const int N = synth.voiceCount();

    hdrDotsPerRow = (uint8_t)((N + 1) / 2);
    hdrDotRadius = (hdrDotsPerRow <= 8) ? 2 : 1;
    hdrDotPitch  = (hdrDotRadius == 2) ? 5 : 3;

    const int cpuTextW   = 4 * 6;
    const int leftMargin = 6;

    hdrCpuX = leftMargin;
    hdrCpuY = (HEADER_H - 8) / 2;
    hdrCpuW = cpuTextW;

    hdrDotsX = hdrCpuX + hdrCpuW + 8;

    int rowsHeight = (hdrDotRadius * 2) * 2 + 2;
    hdrDotsY = (HEADER_H - rowsHeight) / 2 + hdrDotRadius;

    int dotsW = hdrDotsPerRow * hdrDotPitch;
    hdrNameX = hdrDotsX + dotsW + 10;

    const int midiActBlockW = 40;
    midiActX = calBtnX - midiActBlockW - 6;
    midiActY = hdrCpuY;

    meterW = 24;
    meterH = HEADER_H - 8;
    meterX = midiActX - meterW - 8;
    meterY = (HEADER_H - meterH) / 2;
}

void UI::drawCalButton(bool pressed) {
    drawOutlinedButton(calBtnX, calBtnY, calBtnW, calBtnH, "CAL", pressed);
}

void UI::drawHeaderCpu() {
    tft.fillRect(hdrCpuX, 0, hdrCpuW, HEADER_H, TH_BG_DARK);

    float pct = synth.cpuUsagePercent();
    if (pct < 0)  pct = 0;
    if (pct > 99) pct = 99;

    char buf[8];
    snprintf(buf, sizeof(buf), "%2d%%", (int)(pct + 0.5f));

    tft.setTextSize(1);
    uint16_t c = TH_TEXT_NORM;
    if (pct > 70) c = TH_METER_CLIP;
    else if (pct > 50) c = TH_METER_LOUD;
    tft.setTextColor(c);
    tft.setCursor(hdrCpuX, hdrCpuY);
    tft.print(buf);
}

void UI::drawHeaderVoiceDots() {
    const int N = synth.voiceCount();
    for (int i = 0; i < N; i++) {
        int row = (i < hdrDotsPerRow) ? 0 : 1;
        int col = (i < hdrDotsPerRow) ? i : (i - hdrDotsPerRow);
        int cx  = hdrDotsX + col * hdrDotPitch + hdrDotRadius;
        int cy  = hdrDotsY + row * (hdrDotRadius * 2 + 2);

        uint16_t c = TH_VOICE_OFF;
        switch (synth.voiceState(i)) {
            case SynthEngine::VoiceState::Held:      c = TH_VOICE_HELD; break;
            case SynthEngine::VoiceState::Releasing: c = TH_VOICE_REL;  break;
            default: break;
        }
        tft.fillCircle(cx, cy, hdrDotRadius, c);
    }
}

void UI::drawHeaderMeter() {
    float pL = synth.peakLevelL();
    float pR = synth.peakLevelR();

    const float attack  = 0.6f;
    const float release = 0.15f;
    meterPeakL += (pL > meterPeakL) ? (pL - meterPeakL) * attack : (pL - meterPeakL) * release;
    meterPeakR += (pR > meterPeakR) ? (pR - meterPeakR) * attack : (pR - meterPeakR) * release;

    uint32_t now = millis();
    if (pL >= meterHoldL) { meterHoldL = pL; meterHoldMsL = now; }
    else if (now - meterHoldMsL > 800) { meterHoldL *= 0.92f; }

    if (pR >= meterHoldR) { meterHoldR = pR; meterHoldMsR = now; }
    else if (now - meterHoldMsR > 800) { meterHoldR *= 0.92f; }

    int barW = (meterW - 2) / 2;
    int gapX = 2;
    int lx = meterX;
    int rx = meterX + barW + gapX;

    auto drawOne = [&](int bx, float lvl, float hold) {
        tft.fillRect(bx, meterY, barW, meterH, TH_METER_BG);

        if (lvl < 0) lvl = 0;
        if (lvl > 1) lvl = 1;
        if (hold < 0) hold = 0;
        if (hold > 1) hold = 1;

        int fillH = (int)((meterH) * lvl);
        if (fillH > 0) {
            uint16_t c = TH_METER_OK;
            if (lvl >= 0.9f)      c = TH_METER_CLIP;
            else if (lvl >= 0.7f) c = TH_METER_LOUD;
            tft.fillRect(bx, meterY + meterH - fillH, barW, fillH, c);
        }
        if (hold > 0.02f) {
            int hy = meterY + meterH - 1 - (int)((meterH - 1) * hold);
            tft.drawFastHLine(bx, hy, barW, TH_METER_HOLD);
        }
    };

    drawOne(lx, meterPeakL, meterHoldL);
    drawOne(rx, meterPeakR, meterHoldR);
}

void UI::drawMidiActivity() {
    tft.fillRect(midiActX, 0, 40, HEADER_H, TH_BG_DARK);

    struct SrcInfo { MidiSource src; const char* label; uint16_t color; };
    static const SrcInfo srcs[3] = {
        { MidiSource::UsbDevice, "U", TH_MIDI_USBDEV  },
        { MidiSource::Din,       "D", TH_MIDI_DIN     },
        { MidiSource::UsbHost,   "H", TH_MIDI_USBHOST },
    };

    tft.setTextSize(1);
    int x = midiActX;
    for (int i = 0; i < 3; i++) {
        float k = midiActivity.intensity(srcs[i].src);
        bool active = (k > 0.0f);

        tft.setTextColor(active ? TH_TEXT_HI : TH_TEXT_DIM);
        tft.setCursor(x, midiActY);
        tft.print(srcs[i].label);
        x += 6;

        uint16_t c;
        if (k <= 0.0f) {
            c = TH_MIDI_OFF;
        } else {
            uint8_t r = ((srcs[i].color >> 11) & 0x1F);
            uint8_t g = ((srcs[i].color >> 5)  & 0x3F);
            uint8_t b = ( srcs[i].color        & 0x1F);
            r = (uint8_t)(r * k);
            g = (uint8_t)(g * k);
            b = (uint8_t)(b * k);
            c = (uint16_t)((r << 11) | (g << 5) | b);
            if (c == 0) c = TH_MIDI_OFF;
        }
        tft.fillCircle(x + 2, midiActY + 4, 2, c);
        x += 7;
    }
}

void UI::drawHeader() {
    tft.fillRect(0, 0, SCREEN_W, HEADER_H, TH_BG_DARK);

    drawHeaderCpu();
    drawHeaderVoiceDots();

    tft.setTextColor(TH_TEXT_HI);
    tft.setTextSize(1);
    const char* name = synth.patch().name;
    int nameW  = strlen(name) * 6;
    int availW = meterX - hdrNameX - 6;
    int nx     = hdrNameX + (availW - nameW) / 2;
    if (nx < hdrNameX) nx = hdrNameX;
    tft.setCursor(nx, hdrCpuY);
    tft.print(name);

    drawHeaderMeter();
    drawMidiActivity();
    drawCalButton(false);

    drawHRule(HEADER_H - 1);
}

// =============================================================================
// Tabs
// =============================================================================
void UI::drawTabs() {
    tft.fillRect(0, TABS_Y, SCREEN_W, TABS_H, TH_BG_DEEPEST);

    int tabW = SCREEN_W / PAGE_COUNT;
    for (int i = 0; i < PAGE_COUNT; i++) {
        int x = i * tabW;
        bool active = (i == currentPage);

        if (active) {
            tft.fillRect(x + 4, TABS_Y, tabW - 8, TH_ACCENT_BAR_H, TH_ACCENT);
        }

        tft.setTextColor(active ? TH_TEXT_HI : TH_TEXT_DIM);
        tft.setTextSize(1);
        int tw = strlen(pageNames[i]) * 6;
        tft.setCursor(x + (tabW - tw) / 2, TABS_Y + (TABS_H - 8) / 2 + 1);
        tft.print(pageNames[i]);
    }

    drawHRule(TABS_Y + TABS_H - 1);
}

int UI::tabHitTest(int x, int y) const {
    if (y < TABS_Y || y > TABS_Y + TABS_H) return -1;
    int tabW = SCREEN_W / PAGE_COUNT;
    int i = x / tabW;
    if (i >= 0 && i < PAGE_COUNT) return i;
    return -1;
}

// =============================================================================
// Page drawers
// =============================================================================
void UI::drawOscPage() {
    tft.fillRect(0, BODY_Y, SCREEN_W, BODY_H, TH_BG_DEEPEST);
    for (int i = 0; i < pageSliderCount; i++) drawSlimSlider(pageSliders[i]);

    PatchData& p = synth.patch();
    static const char* dests[] = {"OFF","PITCH","PW","FILT"};
    drawDestStrip(dests, 4, p.lfoDest, "LFO DEST");
}

void UI::drawVcfPage() {
    tft.fillRect(0, BODY_Y, SCREEN_W, BODY_H, TH_BG_DEEPEST);
    for (int i = 0; i < pageSliderCount; i++) drawSlimSlider(pageSliders[i]);
}

void UI::drawEnvPage() {
    tft.fillRect(0, BODY_Y, SCREEN_W, BODY_H, TH_BG_DEEPEST);

    // Section labels — centered over each ADSR group
    tft.setTextColor(TH_TEXT_DIM);
    tft.setTextSize(1);
    // AMP group: x=12..152 (4 sliders × 32 + 3 × 4 gap = 140), center ≈ 82
    // "AMP" 18px wide → start at 73
    tft.setCursor( 73, ENV_LABEL_Y); tft.print("AMP");
    // FILTER group: x=174..314, center ≈ 244
    // "FILTER" 36px → start at 226
    tft.setCursor(226, ENV_LABEL_Y); tft.print("FILTER");

    for (int i = 0; i < pageSliderCount; i++) drawSlimSlider(pageSliders[i]);

    // Bottom row: V-AMT inline slider (left) + VEL DEST strip (right)
    static const char* destLabels[] = { "OFF", "VCA", "CUT", "LFO" };
    drawDestStrip(destLabels, 4, synth.patch().velDest, "VEL DEST");

    // V-AMT inline lives left of VEL DEST.
    // Strip starts at x = 320 - 4*44 - 3*2 - 6 = 132, so we have 6..126 available.
    int amtX = 6;
    int amtY = SCREEN_H - DEST_BTN_H - 4;     // align vertically with strip
    int amtW = 120;
    int amtH = DEST_BTN_H;

    InlineSlider vAmt = {
        amtX, amtY, amtW, amtH,
        &synth.patch().velAmount,
        0.0f, 1.0f, false,
        ParamId::VelAmount,
        "V-AMT", nullptr
    };
    drawInlineSlider(vAmt);

    // Cache hit-test rect
    envVelAmtRect = { amtX, amtY, amtW, amtH, true };
}

void UI::drawChorusPage() {
    tft.fillRect(0, BODY_Y, SCREEN_W, BODY_H, TH_BG_DEEPEST);

    const char* modes[] = { "OFF", "CHORUS I", "CHORUS II" };
    uint8_t cur = synth.patch().chorusMode;

    int bw = 80, bh = 22, gap = 6;
    int totalW = 3 * bw + 2 * gap;
    int bx = (SCREEN_W - totalW) / 2;
    int by = BODY_Y + 14;
    for (int i = 0; i < 3; i++) {
        int xx = bx + i * (bw + gap);
        drawOutlinedButton(xx, by, bw, bh, modes[i], cur == i);
    }

    for (int i = 0; i < pageSliderCount; i++) drawSlimSlider(pageSliders[i]);
}

void UI::drawPerfPage() {
    tft.fillRect(0, BODY_Y, SCREEN_W, BODY_H, TH_BG_DEEPEST);

    tft.setTextColor(TH_TEXT_DIM);
    tft.setTextSize(1);
    tft.setCursor(8, BODY_Y + 6); tft.print("ARPEGGIATOR");

    const char* modes[] = { "OFF", "UP", "DN", "UD", "RND" };
    int bw = 44, bh = 18, gap = 4;
    int bx = 8, by = BODY_Y + 20;
    ArpMode cur = arp.getMode();
    for (int i = 0; i < 5; i++) {
        int xx = bx + i * (bw + gap);
        drawOutlinedButton(xx, by, bw, bh, modes[i], cur == i);
    }

    auto drawIncBtn = [&](int x, int y, int w, int h, const char* label) {
        drawOutlinedButton(x, y, w, h, label, false);
    };

    int row2y = by + bh + 10;
    tft.setTextSize(1);
    tft.setTextColor(TH_TEXT_DIM);
    tft.setCursor(8, row2y + 4); tft.print("RATE");
    drawIncBtn(60, row2y, 20, 18, "-");
    tft.setTextColor(TH_TEXT_HI);
    tft.setCursor(88, row2y + 4);
    tft.printf("%.1fHz", arp.getRateHz());
    drawIncBtn(150, row2y, 20, 18, "+");

    int row3y = row2y + 24;
    tft.setTextColor(TH_TEXT_DIM);
    tft.setCursor(8, row3y + 4); tft.print("OCT");
    drawIncBtn(60, row3y, 20, 18, "-");
    tft.setTextColor(TH_TEXT_HI);
    tft.setCursor(88, row3y + 4);
    tft.printf("%d", arp.getOctaves());
    drawIncBtn(150, row3y, 20, 18, "+");

    int sepY = row3y + 26;
    drawHRule(sepY, TH_RULE);

    int midiLabelY = sepY + 6;
    tft.setTextColor(TH_TEXT_DIM);
    tft.setCursor(8, midiLabelY); tft.print("MIDI");

    int row4y = midiLabelY + 14;
    tft.setTextColor(TH_TEXT_DIM);
    tft.setCursor(8, row4y + 4); tft.print("CH");
    drawIncBtn(60, row4y, 20, 18, "-");
    tft.setTextColor(TH_TEXT_HI);
    tft.setCursor(88, row4y + 4);
    uint8_t ch = synth.patch().midiChannel;
    if (ch == 0) tft.print("ALL");
    else         tft.printf("%d", ch);
    drawIncBtn(150, row4y, 20, 18, "+");
}

// =============================================================================
// Patch page
// =============================================================================
static const int PATCH_COLS = 2;
static const int PATCH_ROWS = NUM_PATCH_SLOTS / PATCH_COLS;

static void patchSlotRect(int idx, int& x, int& y, int& w, int& h) {
    const int rightReserved = 70;
    const int gridW = SCREEN_W - rightReserved - 4;
    const int gridH = BODY_H - 6;

    int col = idx % PATCH_COLS;
    int row = idx / PATCH_COLS;

    int cellW = gridW / PATCH_COLS;
    int cellH = gridH / PATCH_ROWS;

    x = 2 + col * cellW;
    y = BODY_Y + 3 + row * cellH;
    w = cellW - 1;
    h = cellH;
}

void UI::drawPatchPage() {
    tft.fillRect(0, BODY_Y, SCREEN_W, BODY_H, TH_BG_DEEPEST);

    char nm[17];
    for (int i = 0; i < NUM_PATCH_SLOTS; i++) {
        int x, y, w, h;
        patchSlotRect(i, x, y, w, h);

        bool has    = patchManager.getPatchName(i, nm);
        bool sel    = (i == selectedSlot);
        bool loaded = (i == loadedSlot);

        if (sel) {
            tft.fillRect(x, y, w, h, TH_ACCENT_DIM);
        }
        if (loaded) {
            tft.fillCircle(x + 4, y + h / 2, 2, sel ? TH_TEXT_HI : TH_ACCENT);
        }

        uint16_t txt = sel ? TH_TEXT_HI : (has ? TH_TEXT_NORM : TH_TEXT_DIM);
        tft.setTextColor(txt);
        tft.setTextSize(1);

        char line[24];
        if (has) {
            snprintf(line, sizeof(line), "%02d  %s", i, nm);
        } else {
            snprintf(line, sizeof(line), "%02d  --", i);
        }
        int maxChars = (w - 14) / 6;
        if (maxChars < 3) maxChars = 3;
        if ((int)strlen(line) > maxChars) line[maxChars] = 0;

        tft.setCursor(x + 10, y + (h - 8) / 2);
        tft.print(line);

        if (!sel) {
            tft.drawFastHLine(x + 2, y + h - 1, w - 4, TH_RULE);
        }
    }

    int colSepX = 2 + (SCREEN_W - 70 - 4) / 2;
    tft.drawFastVLine(colSepX, BODY_Y + 3, BODY_H - 6, TH_RULE);
    tft.drawFastVLine(SCREEN_W - 70, BODY_Y + 2, BODY_H - 4, TH_RULE);

    int bw = 60, bh = 18;
    int bx = SCREEN_W - bw - 5;
    int by1 = BODY_Y + 6;
    int by2 = by1 + bh + 5;
    int by3 = by2 + bh + 5;
    int by4 = by3 + bh + 5;

    drawOutlinedButton(bx, by1, bw, bh, "LOAD");
    drawOutlinedButton(bx, by2, bw, bh, "SAVE");
    drawOutlinedButton(bx, by3, bw, bh, "RENAME");
    drawOutlinedButton(bx, by4, bw, bh, "INIT");
}

int UI::patchSlotHitTest(int px, int py) const {
    for (int i = 0; i < NUM_PATCH_SLOTS; i++) {
        int x,y,w,h; patchSlotRect(i, x, y, w, h);
        if (px >= x && px <= x + w && py >= y && py <= y + h) return i;
    }
    return -1;
}

// =============================================================================
// Status, body, full redraw
// =============================================================================
void UI::showStatus(const char* msg, uint16_t color) {
    int w = 200, h = 28;
    int x = (SCREEN_W - w) / 2, y = (SCREEN_H - h) / 2;

    tft.fillRect(x, y, w, h, TH_BG_PANEL);
    tft.drawRect(x, y, w, h, color);
    tft.setTextColor(TH_TEXT_HI);
    tft.setTextSize(1);
    int tw = strlen(msg) * 6;
    tft.setCursor(x + (w - tw) / 2, y + (h - 8) / 2);
    tft.print(msg);

    delay(700);
    drawBody();
}

void UI::drawBody() {
    // Reset cached inline slider rect; only ENV page sets it
    envVelAmtRect.valid = false;

    buildPageSliders(currentPage);
    switch (currentPage) {
        case PAGE_OSC:    drawOscPage();    break;
        case PAGE_VCF:    drawVcfPage();    break;
        case PAGE_ENV:    drawEnvPage();    break;
        case PAGE_CHORUS: drawChorusPage(); break;
        case PAGE_PERF:   drawPerfPage();   break;
        case PAGE_PATCH:  drawPatchPage();  break;
        default: break;
    }
}

void UI::drawAll() {
    tft.fillScreen(TH_BG_DEEPEST);
    drawHeader();
    drawTabs();
    drawBody();
}

bool UI::inCalButton(int x, int y) const {
    return (x >= calBtnX && x <= calBtnX + calBtnW &&
            y >= calBtnY && y <= calBtnY + calBtnH);
}

int UI::sliderHitTest(int x, int y) const {
    for (int i = 0; i < pageSliderCount; i++) {
        const Slider& s = pageSliders[i];
        if (x >= s.x && x <= s.x + s.w && y >= s.y && y <= s.y + s.h) return i;
    }
    return -1;
}

bool UI::confirmRecalibrate() {
    const int boxW = 220, boxH = 90;
    const int bx = (SCREEN_W - boxW) / 2;
    const int by = (SCREEN_H - boxH) / 2;

    tft.fillRect(bx, by, boxW, boxH, TH_BG_PANEL);
    tft.drawRect(bx, by, boxW, boxH, TH_TEXT_DIM);

    tft.setTextColor(TH_TEXT_HI);
    tft.setTextSize(1);
    tft.setCursor(bx + 12, by + 10); tft.print("RECALIBRATE TOUCH?");
    tft.setTextColor(TH_TEXT_DIM);
    tft.setCursor(bx + 12, by + 24); tft.print("Tap 3 corner crosshairs.");

    int btnW = 64, btnH = 22;
    int yesX = bx + 18, yesY = by + boxH - btnH - 12;
    int noX  = bx + boxW - btnW - 18, noY = yesY;
    drawOutlinedButton(yesX, yesY, btnW, btnH, "YES", true);
    drawOutlinedButton(noX, noY, btnW, btnH, "NO");

    while (ts.touched()) delay(10);
    delay(80);
    uint32_t start = millis();
    while (millis() - start < 10000) {
        if (ts.touched()) {
            TS_Point p = ts.getPoint();
            int16_t sx, sy; touchCal.mapToScreen(p.x, p.y, sx, sy);
            while (ts.touched()) delay(10);
            if (sx >= yesX && sx <= yesX + btnW && sy >= yesY && sy <= yesY + btnH) return true;
            if (sx >= noX  && sx <= noX  + btnW && sy >= noY  && sy <= noY  + btnH) return false;
        }
        delay(10);
    }
    return false;
}

void UI::recalibrateTouch() {
    touchCal.runWizard();
    drawAll();
}

void UI::onPatchSlotTap(int slotIdx) {
    selectedSlot = slotIdx;
    drawPatchPage();
}

// =============================================================================
// Touch dispatch
// =============================================================================
void UI::handleTouch(int x, int y) {
    if (inCalButton(x, y)) {
        drawCalButton(true);
        if (confirmRecalibrate()) recalibrateTouch();
        else drawAll();
        return;
    }
    int t = tabHitTest(x, y);
    if (t >= 0) {
        if ((UIPage)t != currentPage) {
            currentPage = (UIPage)t;
            drawTabs();
            drawBody();
        }
        return;
    }
    int si = sliderHitTest(x, y);
    if (si >= 0) {
        Slider& s = pageSliders[si];
        float norm = 1.0f - (float)(y - s.y) / (float)s.h;
        if (norm < 0) norm = 0;
        if (norm > 1) norm = 1;
        float v;
        if (s.logarithmic && s.min > 0.0f && s.max > 0.0f) {
            v = s.min * powf(s.max / s.min, norm);
        } else {
            v = s.min + norm * (s.max - s.min);
        }
        *s.value = v;
        synth.setParam(s.paramId, v);
        drawSlimSlider(s);
        return;
    }

    if (currentPage == PAGE_OSC) {
        int hit = destStripHit(x, y, 4);
        if (hit >= 0) {
            synth.patch().lfoDest = (uint8_t)hit;
            drawOscPage();
            return;
        }
    } else if (currentPage == PAGE_ENV) {
        // Inline V-AMT slider — check before dest strip
        if (envVelAmtRect.valid &&
            x >= envVelAmtRect.x && x <= envVelAmtRect.x + envVelAmtRect.w &&
            y >= envVelAmtRect.y - 4 && y <= envVelAmtRect.y + envVelAmtRect.h + 4) {
            InlineSlider vAmt = {
                envVelAmtRect.x, envVelAmtRect.y, envVelAmtRect.w, envVelAmtRect.h,
                &synth.patch().velAmount,
                0.0f, 1.0f, false,
                ParamId::VelAmount,
                "V-AMT", nullptr
            };
            inlineSliderApply(vAmt, x);
            drawInlineSlider(vAmt);
            return;
        }
        int hit = destStripHit(x, y, 4);
        if (hit >= 0) {
            synth.patch().velDest = (uint8_t)hit;
            drawEnvPage();
            return;
        }
    } else if (currentPage == PAGE_CHORUS) {
        int bw = 80, bh = 22, gap = 6;
        int totalW = 3 * bw + 2 * gap;
        int bx = (SCREEN_W - totalW) / 2;
        int by = BODY_Y + 14;
        for (int i = 0; i < 3; i++) {
            int xx = bx + i * (bw + gap);
            if (x >= xx && x <= xx + bw && y >= by && y <= by + bh) {
                PatchData& p = synth.patch();
                p.chorusMode = (uint8_t)i;
                if (i == 1)      { p.chorusRate = 0.513f; p.chorusDepth = 22.0f; }
                else if (i == 2) { p.chorusRate = 0.863f; p.chorusDepth = 36.0f; }
                synth.applyPatch(p);
                drawChorusPage();
                return;
            }
        }
    } else if (currentPage == PAGE_PERF) {
        int bw = 44, bh = 18, gap = 4;
        int bx0 = 8, by = BODY_Y + 20;
        if (y >= by && y <= by + bh) {
            for (int i = 0; i < 5; i++) {
                int xx = bx0 + i * (bw + gap);
                if (x >= xx && x <= xx + bw) {
                    arp.setMode((ArpMode)i);
                    drawPerfPage();
                    return;
                }
            }
        }
        int row2y = by + bh + 10;
        if (y >= row2y && y <= row2y + 18) {
            if (x >= 60 && x <= 80)   { arp.setRateHz(arp.getRateHz() - 0.5f); drawPerfPage(); return; }
            if (x >= 150 && x <= 170) { arp.setRateHz(arp.getRateHz() + 0.5f); drawPerfPage(); return; }
        }
        int row3y = row2y + 24;
        if (y >= row3y && y <= row3y + 18) {
            if (x >= 60 && x <= 80)   { arp.setOctaves(arp.getOctaves() - 1); drawPerfPage(); return; }
            if (x >= 150 && x <= 170) { arp.setOctaves(arp.getOctaves() + 1); drawPerfPage(); return; }
        }
        int sepY = row3y + 26;
        int midiLabelY = sepY + 6;
        int row4y = midiLabelY + 14;
        if (y >= row4y && y <= row4y + 18) {
            uint8_t& c = synth.patch().midiChannel;
            if (x >= 60 && x <= 80) {
                if (c == 0) c = 16;
                else c--;
                drawPerfPage();
                return;
            }
            if (x >= 150 && x <= 170) {
                c = (c + 1) % 17;
                drawPerfPage();
                return;
            }
        }
    } else if (currentPage == PAGE_PATCH) {
        int slot = patchSlotHitTest(x, y);
        if (slot >= 0) { onPatchSlotTap(slot); return; }

        int bw = 60, bh = 18;
        int bx = SCREEN_W - bw - 5;
        int by1 = BODY_Y + 6;
        int by2 = by1 + bh + 5;
        int by3 = by2 + bh + 5;
        int by4 = by3 + bh + 5;

        if (x >= bx && x <= bx + bw) {
            if (y >= by1 && y <= by1 + bh) {
                PatchData p;
                if (patchManager.loadPatch(selectedSlot, p)) {
                    synth.applyPatch(p);
                    loadedSlot = selectedSlot;
                    drawHeader();
                    showStatus("LOADED", TH_BTN_OK);
                } else {
                    showStatus("EMPTY SLOT", TH_BTN_WARN);
                }
                return;
            }
            if (y >= by2 && y <= by2 + bh) {
                PatchData& p = synth.patch();
                char seed[17];
                if (p.name[0] == 0 || strcmp(p.name, "INIT PATCH") == 0) {
                    snprintf(seed, sizeof(seed), "SLOT %02d", selectedSlot);
                } else {
                    strncpy(seed, p.name, 16); seed[16] = 0;
                }
                char newName[17];
                bool ok = osKeyboard.edit("Name patch:", seed, newName, sizeof(newName));
                if (!ok) { drawAll(); return; }
                for (int i = strlen(newName) - 1; i >= 0; i--) {
                    if (newName[i] == ' ') newName[i] = 0; else break;
                }
                if (newName[0] == 0) strcpy(newName, "UNTITLED");
                strncpy(p.name, newName, sizeof(p.name) - 1);
                p.name[sizeof(p.name) - 1] = 0;
                bool saved = patchManager.savePatch(selectedSlot, p);
                if (saved) loadedSlot = selectedSlot;
                drawAll();
                showStatus(saved ? "SAVED" : "SAVE FAIL",
                           saved ? TH_BTN_OK : TH_BTN_WARN);
                return;
            }
            if (y >= by3 && y <= by3 + bh) {
                PatchData p;
                if (!patchManager.loadPatch(selectedSlot, p)) {
                    showStatus("EMPTY SLOT", TH_BTN_WARN);
                    return;
                }
                char newName[17];
                bool ok = osKeyboard.edit("Rename patch:", p.name, newName, sizeof(newName));
                if (!ok) { drawAll(); return; }
                for (int i = strlen(newName) - 1; i >= 0; i--) {
                    if (newName[i] == ' ') newName[i] = 0; else break;
                }
                if (newName[0] == 0) strcpy(newName, "UNTITLED");
                strncpy(p.name, newName, sizeof(p.name) - 1);
                p.name[sizeof(p.name) - 1] = 0;
                bool saved = patchManager.savePatch(selectedSlot, p);
                if (loadedSlot == selectedSlot) {
                    strncpy(synth.patch().name, p.name, sizeof(synth.patch().name) - 1);
                    synth.patch().name[sizeof(synth.patch().name) - 1] = 0;
                }
                drawAll();
                showStatus(saved ? "RENAMED" : "RENAME FAIL",
                           saved ? TH_BTN_OK : TH_BTN_WARN);
                return;
            }
            if (y >= by4 && y <= by4 + bh) {
                PatchData init;
                synth.applyPatch(init);
                loadedSlot = -1;
                drawHeader();
                drawBody();
                showStatus("INIT", TH_TEXT_DIM);
                return;
            }
        }
    }
}

void UI::update() {
    static uint32_t lastTouchMs = 0;
    static uint32_t lastMeterMs = 0;
    static char     lastNameShown[17] = "";

    uint32_t now = millis();

    if (ts.touched() && (now - lastTouchMs >= 16)) {
        lastTouchMs = now;
        TS_Point p = ts.getPoint();
        int16_t sx, sy;
        touchCal.mapToScreen(p.x, p.y, sx, sy);
        handleTouch(sx, sy);
    }

    if (now - lastMeterMs >= 100) {
        lastMeterMs = now;
        drawHeaderCpu();
        drawHeaderVoiceDots();
        drawHeaderMeter();
        drawMidiActivity();
    }

    if (strncmp(lastNameShown, synth.patch().name, 16) != 0) {
        strncpy(lastNameShown, synth.patch().name, 16);
        lastNameShown[16] = 0;
        drawHeader();
    }
}
