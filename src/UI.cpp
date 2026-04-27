#include "UI.h"
#include "Config.h"
#include "SynthEngine.h"
#include "PatchManager.h"
#include "TouchCalibration.h"
#include "Arpeggiator.h"
#include "OnScreenKeyboard.h"
#include <ILI9341_t3.h>
#include <XPT2046_Touchscreen.h>
#include "MidiActivity.h"

static ILI9341_t3 tft(TFT_CS, TFT_DC, TFT_RST);
static XPT2046_Touchscreen ts(TOUCH_CS, TOUCH_IRQ);

UI ui;

// ---- Slider description ----
struct Slider {
    const char* label;
    int x, y, w, h;
    float* value;
    float  min, max;
    bool   logarithmic;
    ParamId paramId;
};

static Slider pageSliders[12];
static int    pageSliderCount = 0;

// Layout constants
static constexpr int HEADER_H  = 26;
static constexpr int TABS_Y    = HEADER_H;
static constexpr int TABS_H    = 20;
static constexpr int BODY_Y    = HEADER_H + TABS_H;
static constexpr int BODY_H    = SCREEN_H - BODY_Y;
static constexpr int SLIDER_W  = 30;
static constexpr int SLIDER_H  = 130;
static constexpr int SLIDER_Y  = BODY_Y + 8;

// ENV page uses its own offsets so section labels + VEL DEST strip don't collide
static constexpr int ENV_LABEL_Y  = BODY_Y + 2;
static constexpr int ENV_SLIDER_Y = BODY_Y + 16;
static constexpr int ENV_SLIDER_H = SLIDER_H - 20;

// Shared bottom-row destination strip (used by OSC + ENV pages)
static constexpr int DEST_BTN_W   = 44;
static constexpr int DEST_BTN_H   = 18;
static constexpr int DEST_BTN_GAP = 3;

// Header colors
static constexpr uint16_t DOT_HELD_COLOR      = ILI9341_GREEN;
static constexpr uint16_t DOT_RELEASING_COLOR = ILI9341_YELLOW;
static constexpr uint16_t DOT_OFF_COLOR       = 0x2104;   // very dark grey
static constexpr uint16_t CPU_TEXT_COLOR      = ILI9341_WHITE;
static constexpr uint16_t METER_BG_COLOR   = 0x2104;
static constexpr uint16_t METER_BAR_COLOR  = ILI9341_GREEN;
static constexpr uint16_t METER_LOUD_COLOR = ILI9341_YELLOW;
static constexpr uint16_t METER_CLIP_COLOR = ILI9341_RED;
static constexpr uint16_t METER_HOLD_COLOR = ILI9341_WHITE;
static constexpr uint16_t MIDI_ACT_USBDEV_COLOR = ILI9341_GREEN;
static constexpr uint16_t MIDI_ACT_USBHOST_COLOR = ILI9341_CYAN;
static constexpr uint16_t MIDI_ACT_DIN_COLOR     = ILI9341_MAGENTA;
static constexpr uint16_t MIDI_ACT_OFF_COLOR     = 0x2104;
static constexpr uint16_t MIDI_ACT_LABEL_COLOR   = ILI9341_WHITE;

static const char* pageNames[PAGE_COUNT] = { "PAT", "OSC", "VCF", "ENV", "CHO", "PRF" };

// ---- Page builders ----
static void buildOscSliders() {
    PatchData& p = synth.patch();
    int x = 8;
    pageSliders[0] = { "SAW",   x, SLIDER_Y, SLIDER_W, SLIDER_H, &p.sawLevel,   0.0f, 1.0f, false, ParamId::SawLevel };   x += SLIDER_W + 6;
    pageSliders[1] = { "PUL",   x, SLIDER_Y, SLIDER_W, SLIDER_H, &p.pulseLevel, 0.0f, 1.0f, false, ParamId::PulseLevel }; x += SLIDER_W + 6;
    pageSliders[2] = { "SUB",   x, SLIDER_Y, SLIDER_W, SLIDER_H, &p.subLevel,   0.0f, 1.0f, false, ParamId::SubLevel };   x += SLIDER_W + 6;
    pageSliders[3] = { "PW",    x, SLIDER_Y, SLIDER_W, SLIDER_H, &p.pulseWidth, 0.05f, 0.95f, false, ParamId::PulseWidth }; x += SLIDER_W + 6;
    pageSliders[4] = { "LFO-R", x, SLIDER_Y, SLIDER_W, SLIDER_H, &p.lfoRate,    0.05f, 20.0f, true, ParamId::LfoRate };   x += SLIDER_W + 6;
    pageSliders[5] = { "LFO-D", x, SLIDER_Y, SLIDER_W, SLIDER_H, &p.lfoDepth,   0.0f, 1.0f, false, ParamId::LfoDepth };   x += SLIDER_W + 6;
    pageSliders[6] = { "GLIDE", x, SLIDER_Y, SLIDER_W, SLIDER_H, &p.glideMs,    0.0f, 1000.0f, false, ParamId::GlideMs };
    pageSliderCount = 7;
}
static void buildVcfSliders() {
    PatchData& p = synth.patch();
    int x = 10;
    pageSliders[0] = { "HPF", x, SLIDER_Y, SLIDER_W, SLIDER_H, &p.hpfCutoff, 20.0f, 1000.0f, true, ParamId::HpfCutoff }; x += SLIDER_W + 8;
    pageSliders[1] = { "CUT", x, SLIDER_Y, SLIDER_W, SLIDER_H, &p.cutoff,    40.0f, 8000.0f, true, ParamId::Cutoff };    x += SLIDER_W + 8;
    pageSliders[2] = { "RES", x, SLIDER_Y, SLIDER_W, SLIDER_H, &p.resonance, 0.7f,  5.0f,    false, ParamId::Resonance }; x += SLIDER_W + 8;
    pageSliders[3] = { "ENV", x, SLIDER_Y, SLIDER_W, SLIDER_H, &p.envAmount, 0.0f,  1.0f,    false, ParamId::EnvAmount };
    pageSliderCount = 4;
}
static void buildEnvSliders() {
    PatchData& p = synth.patch();
    int x = 10;
    pageSliders[0] = { "A-A",  x, ENV_SLIDER_Y, SLIDER_W, ENV_SLIDER_H, &p.ampA, 0.0f, 3000.0f, true, ParamId::AmpA }; x += SLIDER_W + 4;
    pageSliders[1] = { "A-D",  x, ENV_SLIDER_Y, SLIDER_W, ENV_SLIDER_H, &p.ampD, 0.0f, 3000.0f, true, ParamId::AmpD }; x += SLIDER_W + 4;
    pageSliders[2] = { "A-S",  x, ENV_SLIDER_Y, SLIDER_W, ENV_SLIDER_H, &p.ampS, 0.0f, 1.0f,    false, ParamId::AmpS }; x += SLIDER_W + 4;
    pageSliders[3] = { "A-R",  x, ENV_SLIDER_Y, SLIDER_W, ENV_SLIDER_H, &p.ampR, 0.0f, 5000.0f, true, ParamId::AmpR }; x += SLIDER_W + 12;
    pageSliders[4] = { "F-A",  x, ENV_SLIDER_Y, SLIDER_W, ENV_SLIDER_H, &p.fltA, 0.0f, 3000.0f, true, ParamId::FltA }; x += SLIDER_W + 4;
    pageSliders[5] = { "F-D",  x, ENV_SLIDER_Y, SLIDER_W, ENV_SLIDER_H, &p.fltD, 0.0f, 3000.0f, true, ParamId::FltD }; x += SLIDER_W + 4;
    pageSliders[6] = { "F-S",  x, ENV_SLIDER_Y, SLIDER_W, ENV_SLIDER_H, &p.fltS, 0.0f, 1.0f,    false, ParamId::FltS }; x += SLIDER_W + 4;
    pageSliders[7] = { "F-R",  x, ENV_SLIDER_Y, SLIDER_W, ENV_SLIDER_H, &p.fltR, 0.0f, 5000.0f, true, ParamId::FltR }; x += SLIDER_W + 12;
    pageSliders[8] = { "V-AMT",x, ENV_SLIDER_Y, SLIDER_W, ENV_SLIDER_H, &p.velAmount, 0.0f, 1.0f, false, ParamId::VelAmount };
    pageSliderCount = 9;
}
static void buildChorusSliders() {
    PatchData& p = synth.patch();
    int x = 20;
    int y = BODY_Y + 55;
    pageSliders[0] = { "RATE",  x, y, SLIDER_W, SLIDER_H - 20, &p.chorusRate,  0.05f, 8.0f, true, ParamId::ChorusRate };  x += SLIDER_W + 12;
    pageSliders[1] = { "DEPTH", x, y, SLIDER_W, SLIDER_H - 20, &p.chorusDepth, 0.0f, 80.0f, false, ParamId::ChorusDepth };
    pageSliderCount = 2;
}
static void buildPerfSliders() {
    pageSliderCount = 0;
}
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

static void drawSliderAt(const Slider& s) {
    tft.drawRect(s.x, s.y, s.w, s.h, ILI9341_DARKGREY);
    tft.fillRect(s.x+1, s.y+1, s.w-2, s.h-2, ILI9341_BLACK);
    float v = *s.value;
    float norm;
    if (s.logarithmic && s.min > 0.0f && s.max > 0.0f) {
        norm = logf(v / s.min) / logf(s.max / s.min);
    } else {
        norm = (v - s.min) / (s.max - s.min);
    }
    if (norm < 0) norm = 0;
    if (norm > 1) norm = 1;
    int fillH = (int)((s.h - 2) * norm);
    tft.fillRect(s.x+1, s.y + s.h - 1 - fillH, s.w-2, fillH, ILI9341_ORANGE);
    tft.setTextColor(ILI9341_WHITE);
    tft.setTextSize(1);
    tft.setCursor(s.x + 1, s.y + s.h + 3);
    tft.print(s.label);
}

// ---- Shared dest-strip (OSC + ENV) ----
static void destStripLayout(int& bx, int& by, int count) {
    bx = SCREEN_W - count*DEST_BTN_W - (count-1)*DEST_BTN_GAP - 6;
    by = SCREEN_H - DEST_BTN_H - 3;
}
static void drawDestStrip(const char* const* labels, int count, int current, const char* title) {
    int bx, by;
    destStripLayout(bx, by, count);
    tft.setTextSize(1);
    tft.setTextColor(ILI9341_WHITE);
    tft.setCursor(bx, by - 10);
    tft.print(title);
    for (int i = 0; i < count; i++) {
        int xx = bx + i*(DEST_BTN_W + DEST_BTN_GAP);
        uint16_t bg = (current == i) ? ILI9341_ORANGE : ILI9341_DARKGREY;
        tft.fillRoundRect(xx, by, DEST_BTN_W, DEST_BTN_H, 3, bg);
        tft.drawRoundRect(xx, by, DEST_BTN_W, DEST_BTN_H, 3, ILI9341_WHITE);
        tft.setTextColor(ILI9341_WHITE);
        int tw = strlen(labels[i]) * 6;
        tft.setCursor(xx + (DEST_BTN_W - tw)/2, by + (DEST_BTN_H - 8)/2);
        tft.print(labels[i]);
    }
}
static int destStripHit(int x, int y, int count) {
    int bx, by;
    destStripLayout(bx, by, count);
    if (y < by || y > by + DEST_BTN_H) return -1;
    for (int i = 0; i < count; i++) {
        int xx = bx + i*(DEST_BTN_W + DEST_BTN_GAP);
        if (x >= xx && x <= xx + DEST_BTN_W) return i;
    }
    return -1;
}

// ---- UI init ----
void UI::begin() {
    tft.begin();
    tft.setRotation(DISPLAY_ROTATION);
    tft.fillScreen(ILI9341_BLACK);

    ts.begin();
    ts.setRotation(0);

    touchCal.begin(&tft, &ts);
    bool needCal = FORCE_TOUCH_RECAL;
    if (!needCal && !touchCal.loadFromSD()) needCal = true;
    if (!needCal && RECAL_ON_BOOT_TOUCH && ts.touched()) needCal = true;
    if (needCal) touchCal.runWizard();

    calBtnW = 36; calBtnH = 16;
    calBtnX = SCREEN_W - calBtnW - 4;
    calBtnY = 3;

    computeHeaderLayout();

    osKeyboard.begin(&tft, &ts);

    drawAll();
}

// ---- Header: CPU % + voice dots + patch name + CAL button ----

void UI::computeHeaderLayout() {
    const int N = synth.voiceCount();

    hdrDotsPerRow = (uint8_t)((N + 1) / 2);
    hdrDotRadius = (hdrDotsPerRow <= 8) ? 2 : 1;
    hdrDotPitch  = (hdrDotRadius == 2) ? 6 : 4;

    const int cpuTextW  = 7 * 6;
    const int leftMargin = 4;

    hdrCpuX = leftMargin;
    hdrCpuY = (HEADER_H - 8) / 2;
    hdrCpuW = cpuTextW;

    hdrDotsX = hdrCpuX + hdrCpuW + 6;

    int rowsHeight = (hdrDotRadius * 2) * 2 + 2;
    hdrDotsY = (HEADER_H - rowsHeight) / 2 + hdrDotRadius;

    int dotsW = hdrDotsPerRow * hdrDotPitch;
    hdrNameX = hdrDotsX + dotsW + 8;

    // MIDI activity indicator block: 3 dots with 1-char labels above them.
    // Goes immediately left of the CAL button.
    // Each column is 10 px wide, 3 columns + 2 gaps = 30 px + a little padding.
    const int midiActBlockW = 32;
    midiActX = calBtnX - midiActBlockW - 6;
    midiActY = 2;   // near top of header

    // Stereo meter: placed immediately left of the MIDI activity block
    meterW = 32;
    meterH = HEADER_H - 8;
    meterX = midiActX - meterW - 6;
    meterY = (HEADER_H - meterH) / 2;
}

void UI::drawCalButton(bool pressed) {
    uint16_t bg = pressed ? ILI9341_ORANGE : ILI9341_DARKGREY;
    tft.fillRoundRect(calBtnX, calBtnY, calBtnW, calBtnH, 3, bg);
    tft.drawRoundRect(calBtnX, calBtnY, calBtnW, calBtnH, 3, ILI9341_WHITE);
    tft.setTextColor(ILI9341_WHITE);
    tft.setTextSize(1);
    int tw = 3 * 6;
    tft.setCursor(calBtnX + (calBtnW - tw)/2, calBtnY + (calBtnH - 8)/2);
    tft.print("CAL");
}

void UI::drawHeaderCpu() {
    // Clear CPU text area so old digits don't ghost.
    tft.fillRect(hdrCpuX, 0, hdrCpuW, HEADER_H, ILI9341_NAVY);

    float pct = synth.cpuUsagePercent();
    if (pct < 0)  pct = 0;
    if (pct > 99) pct = 99;

    char buf[12];
    snprintf(buf, sizeof(buf), "CPU %2d%%", (int)(pct + 0.5f));

    tft.setTextSize(1);
    tft.setTextColor(CPU_TEXT_COLOR);
    tft.setCursor(hdrCpuX, hdrCpuY);
    tft.print(buf);
}

void UI::drawMidiActivity() {
    // Clear the block
    tft.fillRect(midiActX, 0, 32, HEADER_H, ILI9341_NAVY);

    struct SrcInfo {
        MidiSource src;
        const char* label;
        uint16_t color;
    };
    static const SrcInfo srcs[3] = {
        { MidiSource::UsbDevice, "U", MIDI_ACT_USBDEV_COLOR  },
        { MidiSource::Din,       "D", MIDI_ACT_DIN_COLOR     },
        { MidiSource::UsbHost,   "H", MIDI_ACT_USBHOST_COLOR },
    };

    const int colW    = 10;
    const int radius  = 2;
    const int labelY  = midiActY;
    const int dotY    = midiActY + 12;    // below the label

    tft.setTextSize(1);

    for (int i = 0; i < 3; i++) {
        int cx = midiActX + i * colW + colW / 2;

        // Label
        tft.setTextColor(MIDI_ACT_LABEL_COLOR);
        tft.setCursor(cx - 3, labelY);    // 3 px = half of 6-px char width
        tft.print(srcs[i].label);

        // Dot: color fades with recent activity
        float k = midiActivity.intensity(srcs[i].src);
        uint16_t c;
        if (k <= 0.0f) {
            c = MIDI_ACT_OFF_COLOR;
        } else {
            // Blend srcs[i].color toward black as k drops
            uint8_t r = ((srcs[i].color >> 11) & 0x1F);
            uint8_t g = ((srcs[i].color >> 5)  & 0x3F);
            uint8_t b = ( srcs[i].color        & 0x1F);
            r = (uint8_t)(r * k);
            g = (uint8_t)(g * k);
            b = (uint8_t)(b * k);
            c = (uint16_t)((r << 11) | (g << 5) | b);
            if (c == 0) c = MIDI_ACT_OFF_COLOR;
        }
        tft.fillCircle(cx, dotY, radius, c);
    }
}

void UI::drawHeaderMeter() {
    // Consume peak readings
    float pL = synth.peakLevelL();
    float pR = synth.peakLevelR();

    // Smooth the displayed level (fast attack, slow release)
    const float attack  = 0.6f;
    const float release = 0.15f;

    meterPeakL += (pL > meterPeakL) ? (pL - meterPeakL) * attack
                                    : (pL - meterPeakL) * release;
    meterPeakR += (pR > meterPeakR) ? (pR - meterPeakR) * attack
                                    : (pR - meterPeakR) * release;

    // Peak-hold: latch the highest value, then decay after 800 ms
    uint32_t now = millis();
    if (pL >= meterHoldL) { meterHoldL = pL; meterHoldMsL = now; }
    else if (now - meterHoldMsL > 800) { meterHoldL *= 0.92f; }

    if (pR >= meterHoldR) { meterHoldR = pR; meterHoldMsR = now; }
    else if (now - meterHoldMsR > 800) { meterHoldR *= 0.92f; }

    // Two vertical bars side by side
    int barW = (meterW - 2) / 2;
    int gapX = 2;
    int lx = meterX;
    int rx = meterX + barW + gapX;

    auto drawOne = [&](int bx, float lvl, float hold) {
        // Background
        tft.fillRect(bx, meterY, barW, meterH, METER_BG_COLOR);
        tft.drawRect(bx, meterY, barW, meterH, ILI9341_DARKGREY);

        // Clamp
        if (lvl < 0) lvl = 0;
        if (lvl > 1) lvl = 1;
        if (hold < 0) hold = 0;
        if (hold > 1) hold = 1;

        int fillH = (int)((meterH - 2) * lvl);
        if (fillH > 0) {
            // Color zones: green <0.7, yellow <0.9, red >=0.9
            uint16_t c = METER_BAR_COLOR;
            if (lvl >= 0.9f)      c = METER_CLIP_COLOR;
            else if (lvl >= 0.7f) c = METER_LOUD_COLOR;
            tft.fillRect(bx + 1, meterY + meterH - 1 - fillH, barW - 2, fillH, c);
        }

        // Peak-hold tick
        if (hold > 0.02f) {
            int hy = meterY + meterH - 1 - (int)((meterH - 2) * hold);
            tft.drawFastHLine(bx + 1, hy, barW - 2, METER_HOLD_COLOR);
        }
    };

    drawOne(lx, meterPeakL, meterHoldL);
    drawOne(rx, meterPeakR, meterHoldR);
}

void UI::drawHeaderVoiceDots() {
    const int N = synth.voiceCount();
    for (int i = 0; i < N; i++) {
        int row = (i < hdrDotsPerRow) ? 0 : 1;
        int col = (i < hdrDotsPerRow) ? i : (i - hdrDotsPerRow);
        int cx  = hdrDotsX + col * hdrDotPitch + hdrDotRadius;
        int cy  = hdrDotsY + row * (hdrDotRadius * 2 + 2);

        uint16_t c = DOT_OFF_COLOR;
        switch (synth.voiceState(i)) {
            case SynthEngine::VoiceState::Held:      c = DOT_HELD_COLOR;      break;
            case SynthEngine::VoiceState::Releasing: c = DOT_RELEASING_COLOR; break;
            case SynthEngine::VoiceState::Idle:
            default:                                 c = DOT_OFF_COLOR;       break;
        }
        tft.fillCircle(cx, cy, hdrDotRadius, c);
    }
}

void UI::drawHeader() {
    tft.fillRect(0, 0, SCREEN_W, HEADER_H, ILI9341_NAVY);

    drawHeaderCpu();
    drawHeaderVoiceDots();

    // Patch name centered in the remaining zone
    tft.setTextColor(ILI9341_WHITE);
    tft.setTextSize(1);

    const char* name = synth.patch().name;
    int nameW  = strlen(name) * 6;
    int availW = meterX - hdrNameX - 6;
    int nx     = hdrNameX + (availW - nameW) / 2;
    if (nx < hdrNameX) nx = hdrNameX;

    tft.setCursor(nx, hdrCpuY);
    tft.print(name);

    drawCalButton(false);
    drawHeaderMeter();
}

void UI::drawTabs() {
    tft.fillRect(0, TABS_Y, SCREEN_W, TABS_H, ILI9341_BLACK);
    int tabW = SCREEN_W / PAGE_COUNT;
    for (int i = 0; i < PAGE_COUNT; i++) {
        int x = i * tabW;
        bool active = (i == currentPage);
        uint16_t bg = active ? ILI9341_ORANGE : ILI9341_DARKGREY;
        tft.fillRect(x+1, TABS_Y+1, tabW-2, TABS_H-2, bg);
        tft.drawRect(x,   TABS_Y,   tabW,   TABS_H,   ILI9341_WHITE);
        tft.setTextColor(ILI9341_WHITE);
        tft.setTextSize(1);
        int tw = strlen(pageNames[i]) * 6;
        tft.setCursor(x + (tabW - tw)/2, TABS_Y + (TABS_H - 8)/2);
        tft.print(pageNames[i]);
    }
}

int UI::tabHitTest(int x, int y) const {
    if (y < TABS_Y || y > TABS_Y + TABS_H) return -1;
    int tabW = SCREEN_W / PAGE_COUNT;
    int i = x / tabW;
    if (i >= 0 && i < PAGE_COUNT) return i;
    return -1;
}

// ---- Page drawers ----
void UI::drawOscPage() {
    tft.fillRect(0, BODY_Y, SCREEN_W, BODY_H, ILI9341_BLACK);
    for (int i = 0; i < pageSliderCount; i++) drawSliderAt(pageSliders[i]);

    PatchData& p = synth.patch();
    static const char* dests[] = {"OFF","PIT","PW","FIL"};
    drawDestStrip(dests, 4, p.lfoDest, "LFO DEST");
}

void UI::drawVcfPage() {
    tft.fillRect(0, BODY_Y, SCREEN_W, BODY_H, ILI9341_BLACK);
    for (int i = 0; i < pageSliderCount; i++) drawSliderAt(pageSliders[i]);
}

void UI::drawEnvPage() {
    tft.fillRect(0, BODY_Y, SCREEN_W, BODY_H, ILI9341_BLACK);

    tft.setTextColor(ILI9341_CYAN);
    tft.setTextSize(1);
    tft.setCursor(10,  ENV_LABEL_Y); tft.print("AMP ENV");
    tft.setCursor(178, ENV_LABEL_Y); tft.print("FILTER ENV");
    tft.setCursor(290, ENV_LABEL_Y); tft.print("VEL");

    for (int i = 0; i < pageSliderCount; i++) drawSliderAt(pageSliders[i]);

    static const char* labels[] = { "OFF", "VCA", "CUT", "LFO" };
    drawDestStrip(labels, 4, synth.patch().velDest, "VEL DEST");
}

void UI::drawChorusPage() {
    tft.fillRect(0, BODY_Y, SCREEN_W, BODY_H, ILI9341_BLACK);
    const char* modes[] = { "OFF", "CHORUS I", "CHORUS II" };
    uint8_t cur = synth.patch().chorusMode;

    int bw = 76, bh = 24, gap = 6;
    int totalW = 3*bw + 2*gap;
    int bx = (SCREEN_W - totalW)/2;
    int by = BODY_Y + 8;
    tft.setTextSize(1);
    for (int i = 0; i < 3; i++) {
        uint16_t bg = (cur == i) ? ILI9341_ORANGE : ILI9341_DARKGREY;
        int xx = bx + i*(bw + gap);
        tft.fillRoundRect(xx, by, bw, bh, 4, bg);
        tft.drawRoundRect(xx, by, bw, bh, 4, ILI9341_WHITE);
        tft.setTextColor(ILI9341_WHITE);
        int tw = strlen(modes[i]) * 6;
        tft.setCursor(xx + (bw - tw)/2, by + (bh - 8)/2);
        tft.print(modes[i]);
    }

    for (int i = 0; i < pageSliderCount; i++) drawSliderAt(pageSliders[i]);
}

void UI::drawPerfPage() {
    tft.fillRect(0, BODY_Y, SCREEN_W, BODY_H, ILI9341_BLACK);
    tft.setTextColor(ILI9341_CYAN);
    tft.setTextSize(1);
    tft.setCursor(10, BODY_Y + 4);
    tft.print("ARPEGGIATOR");

    const char* modes[] = { "OFF", "UP", "DN", "UD", "RND" };
    int bw = 40, bh = 20, gap = 6;
    int bx = 10, by = BODY_Y + 22;
    ArpMode cur = arp.getMode();
    tft.setTextSize(1);
    for (int i = 0; i < 5; i++) {
        int xx = bx + i*(bw+gap);
        uint16_t bg = (cur == i) ? ILI9341_ORANGE : ILI9341_DARKGREY;
        tft.fillRoundRect(xx, by, bw, bh, 3, bg);
        tft.drawRoundRect(xx, by, bw, bh, 3, ILI9341_WHITE);
        tft.setTextColor(ILI9341_WHITE);
        int tw = strlen(modes[i]) * 6;
        tft.setCursor(xx + (bw - tw)/2, by + (bh - 8)/2);
        tft.print(modes[i]);
    }

    int row2y = by + bh + 10;
    auto drawIncBtn = [&](int x, int y, int w, int h, const char* label) {
        tft.fillRoundRect(x, y, w, h, 3, ILI9341_DARKGREY);
        tft.drawRoundRect(x, y, w, h, 3, ILI9341_WHITE);
        tft.setTextColor(ILI9341_WHITE); tft.setTextSize(1);
        int tw = strlen(label) * 6;
        tft.setCursor(x + (w - tw)/2, y + (h - 8)/2);
        tft.print(label);
    };

    tft.setTextSize(1); tft.setTextColor(ILI9341_WHITE);
    tft.setCursor(10, row2y + 4); tft.print("RATE");
    drawIncBtn(64, row2y, 22, 20, "-");
    tft.setCursor(94, row2y + 4);
    tft.printf("%.1fHz", arp.getRateHz());
    drawIncBtn(160, row2y, 22, 20, "+");

    int row3y = row2y + 28;
    tft.setCursor(10, row3y + 4); tft.print("OCT");
    drawIncBtn(64, row3y, 22, 20, "-");
    tft.setCursor(94, row3y + 4);
    tft.printf("%d", arp.getOctaves());
    drawIncBtn(160, row3y, 22, 20, "+");
}

// ---- Patch page ----
static const int PATCH_COLS = 2;
static const int PATCH_ROWS = NUM_PATCH_SLOTS / PATCH_COLS;

static void patchSlotRect(int idx, int& x, int& y, int& w, int& h) {
    const int rightReserved = 72;
    const int gridW = SCREEN_W - rightReserved - 4;
    const int gridH = BODY_H - 6;

    int col = idx % PATCH_COLS;
    int row = idx / PATCH_COLS;

    int cellW = gridW / PATCH_COLS;
    int cellH = gridH / PATCH_ROWS;

    x = 3 + col * cellW;
    y = BODY_Y + 3 + row * cellH;
    w = cellW - 2;
    h = cellH - 1;
}

void UI::drawPatchPage() {
    tft.fillRect(0, BODY_Y, SCREEN_W, BODY_H, ILI9341_BLACK);

    char nm[17];
    for (int i = 0; i < NUM_PATCH_SLOTS; i++) {
        int x, y, w, h;
        patchSlotRect(i, x, y, w, h);

        bool has    = patchManager.getPatchName(i, nm);
        bool sel    = (i == selectedSlot);
        bool loaded = (i == loadedSlot);

        uint16_t bg   = sel    ? ILI9341_ORANGE : (has ? ILI9341_NAVY : 0x2104);
        uint16_t edge = loaded ? ILI9341_CYAN   : ILI9341_DARKGREY;

        tft.fillRoundRect(x, y, w, h, 2, bg);
        tft.drawRoundRect(x, y, w, h, 2, edge);

        tft.setTextColor(sel ? ILI9341_BLACK : ILI9341_WHITE);
        tft.setTextSize(1);

        char line[24];
        if (has) {
            snprintf(line, sizeof(line), "%02d %s", i, nm);
        } else {
            snprintf(line, sizeof(line), "%02d --", i);
        }

        int maxChars = (w - 6) / 6;
        if (maxChars < 3) maxChars = 3;
        if ((int)strlen(line) > maxChars) line[maxChars] = 0;

        tft.setCursor(x + 4, y + (h - 8) / 2);
        tft.print(line);
    }

    // Action buttons
    int bw = 60, bh = 20;
    int bx = SCREEN_W - bw - 4;
    int by1 = BODY_Y + 6;
    int by2 = by1 + bh + 4;
    int by3 = by2 + bh + 4;
    int by4 = by3 + bh + 4;

    auto drawBtn = [&](int y, const char* label, uint16_t bg) {
        tft.fillRoundRect(bx, y, bw, bh, 3, bg);
        tft.drawRoundRect(bx, y, bw, bh, 3, ILI9341_WHITE);
        tft.setTextColor(ILI9341_WHITE);
        tft.setTextSize(1);
        int tw = strlen(label) * 6;
        tft.setCursor(bx + (bw - tw)/2, y + (bh - 8)/2);
        tft.print(label);
    };
    drawBtn(by1, "LOAD",   ILI9341_DARKGREEN);
    drawBtn(by2, "SAVE",   ILI9341_MAROON);
    drawBtn(by3, "RENAME", ILI9341_NAVY);
    drawBtn(by4, "INIT",   ILI9341_DARKGREY);
}

int UI::patchSlotHitTest(int px, int py) const {
    for (int i = 0; i < NUM_PATCH_SLOTS; i++) {
        int x,y,w,h; patchSlotRect(i, x, y, w, h);
        if (px >= x && px <= x + w && py >= y && py <= y + h) return i;
    }
    return -1;
}

void UI::showStatus(const char* msg, uint16_t color) {
    int w = SCREEN_W - 20, h = 32;
    int x = 10, y = SCREEN_H/2 - h/2;
    tft.fillRoundRect(x, y, w, h, 5, color);
    tft.drawRoundRect(x, y, w, h, 5, ILI9341_WHITE);
    tft.setTextColor(ILI9341_WHITE);
    tft.setTextSize(1);
    int tw = strlen(msg) * 6;
    tft.setCursor(x + (w - tw)/2, y + (h - 8)/2);
    tft.print(msg);
    delay(700);
    drawBody();
}

void UI::drawBody() {
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
    tft.fillScreen(ILI9341_BLACK);
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
    const int boxW = 220, boxH = 100;
    const int bx = (SCREEN_W - boxW) / 2;
    const int by = (SCREEN_H - boxH) / 2;
    tft.fillRoundRect(bx, by, boxW, boxH, 6, ILI9341_NAVY);
    tft.drawRoundRect(bx, by, boxW, boxH, 6, ILI9341_WHITE);
    tft.setTextColor(ILI9341_WHITE);
    tft.setTextSize(1);
    tft.setCursor(bx + 12, by + 12); tft.print("Recalibrate touch?");
    tft.setCursor(bx + 12, by + 28); tft.print("Tap 3 corner crosshairs.");
    int btnW = 64, btnH = 26;
    int yesX = bx + 18, yesY = by + boxH - btnH - 12;
    int noX  = bx + boxW - btnW - 18, noY = yesY;
    tft.fillRoundRect(yesX, yesY, btnW, btnH, 4, ILI9341_DARKGREEN);
    tft.drawRoundRect(yesX, yesY, btnW, btnH, 4, ILI9341_WHITE);
    tft.setTextColor(ILI9341_WHITE);
    tft.setCursor(yesX + (btnW - 18)/2, yesY + (btnH - 8)/2); tft.print("YES");
    tft.fillRoundRect(noX, noY, btnW, btnH, 4, ILI9341_MAROON);
    tft.drawRoundRect(noX, noY, btnW, btnH, 4, ILI9341_WHITE);
    tft.setCursor(noX + (btnW - 12)/2, noY + (btnH - 8)/2); tft.print("NO");

    while (ts.touched()) delay(10);
    delay(80);
    uint32_t start = millis();
    while (millis() - start < 10000) {
        if (ts.touched()) {
            TS_Point p = ts.getPoint();
            int16_t sx, sy; touchCal.mapToScreen(p.x, p.y, sx, sy);
            while (ts.touched()) delay(10);
            if (sx >= yesX && sx <= yesX+btnW && sy >= yesY && sy <= yesY+btnH) return true;
            if (sx >= noX  && sx <= noX +btnW && sy >= noY  && sy <= noY +btnH) return false;
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
        drawSliderAt(s);
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
        int hit = destStripHit(x, y, 4);
        if (hit >= 0) {
            synth.patch().velDest = (uint8_t)hit;
            drawEnvPage();
            return;
        }
    } else if (currentPage == PAGE_CHORUS) {
        int bw = 76, bh = 24, gap = 6;
        int totalW = 3*bw + 2*gap;
        int bx = (SCREEN_W - totalW)/2;
        int by = BODY_Y + 8;
        for (int i = 0; i < 3; i++) {
            int xx = bx + i*(bw + gap);
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
        int bw = 40, bh = 20, gap = 6;
        int bx0 = 10, by = BODY_Y + 22;
        if (y >= by && y <= by + bh) {
            for (int i = 0; i < 5; i++) {
                int xx = bx0 + i*(bw+gap);
                if (x >= xx && x <= xx + bw) {
                    arp.setMode((ArpMode)i);
                    drawPerfPage();
                    return;
                }
            }
        }
        int row2y = by + bh + 10;
        if (y >= row2y && y <= row2y + 20) {
            if (x >= 64 && x <= 86)   { arp.setRateHz(arp.getRateHz() - 0.5f); drawPerfPage(); return; }
            if (x >= 160 && x <= 182) { arp.setRateHz(arp.getRateHz() + 0.5f); drawPerfPage(); return; }
        }
        int row3y = row2y + 28;
        if (y >= row3y && y <= row3y + 20) {
            if (x >= 64 && x <= 86)   { arp.setOctaves(arp.getOctaves() - 1); drawPerfPage(); return; }
            if (x >= 160 && x <= 182) { arp.setOctaves(arp.getOctaves() + 1); drawPerfPage(); return; }
        }
    } else if (currentPage == PAGE_PATCH) {
        int slot = patchSlotHitTest(x, y);
        if (slot >= 0) { onPatchSlotTap(slot); return; }

        int bw = 60, bh = 20;
        int bx = SCREEN_W - bw - 4;
        int by1 = BODY_Y + 6;
        int by2 = by1 + bh + 4;
        int by3 = by2 + bh + 4;
        int by4 = by3 + bh + 4;

        if (x >= bx && x <= bx + bw) {
            // LOAD
            if (y >= by1 && y <= by1 + bh) {
                PatchData p;
                if (patchManager.loadPatch(selectedSlot, p)) {
                    synth.applyPatch(p);
                    loadedSlot = selectedSlot;
                    drawHeader();
                    showStatus("LOADED", ILI9341_DARKGREEN);
                } else {
                    showStatus("EMPTY SLOT", ILI9341_MAROON);
                }
                return;
            }
            // SAVE
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
                           saved ? ILI9341_DARKGREEN : ILI9341_MAROON);
                return;
            }
            // RENAME
            if (y >= by3 && y <= by3 + bh) {
                PatchData p;
                if (!patchManager.loadPatch(selectedSlot, p)) {
                    showStatus("EMPTY SLOT", ILI9341_MAROON);
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
                           saved ? ILI9341_DARKGREEN : ILI9341_MAROON);
                return;
            }
            // INIT
            if (y >= by4 && y <= by4 + bh) {
                PatchData init;
                synth.applyPatch(init);
                loadedSlot = -1;
                drawHeader();
                drawBody();
                showStatus("INIT PATCH", ILI9341_NAVY);
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

    // CPU + voice dots refresh at ~10 Hz
    if (now - lastMeterMs >= 100) {
        lastMeterMs = now;
        drawHeaderCpu();
        drawHeaderVoiceDots();
        drawHeaderMeter();
        drawMidiActivity();    // <-- NEW
    }

    // Full header redraw only when the patch name actually changes
    if (strncmp(lastNameShown, synth.patch().name, 16) != 0) {
        strncpy(lastNameShown, synth.patch().name, 16);
        lastNameShown[16] = 0;
        drawHeader();
    }
}
