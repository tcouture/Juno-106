#include "UI.h"
#include "Config.h"
#include "SynthEngine.h"
#include "PatchManager.h"
#include "TouchCalibration.h"
#include <ILI9341_t3.h>
#include <XPT2046_Touchscreen.h>
#include "OnScreenKeyboard.h"

static ILI9341_t3 tft(TFT_CS, TFT_DC, TFT_RST);
static XPT2046_Touchscreen ts(TOUCH_CS, TOUCH_IRQ);

UI ui;

// ------------------- Slider description -------------------
struct Slider {
    const char* label;
    int x, y, w, h;
    float* value;
    float  min, max;
    bool   logarithmic = false;
};

// Per-page slider arrays, rebuilt whenever page changes or patch changes.
static Slider pageSliders[10];
static int    pageSliderCount = 0;

// Layout constants
static constexpr int HEADER_H  = 30;
static constexpr int TABS_Y    = HEADER_H;
static constexpr int TABS_H    = 24;
static constexpr int BODY_Y    = HEADER_H + TABS_H;
static constexpr int BODY_H    = SCREEN_H - BODY_Y;
static constexpr int SLIDER_W  = 34;
static constexpr int SLIDER_H  = 130;
static constexpr int SLIDER_Y  = BODY_Y + 10;

static const char* pageNames[PAGE_COUNT] = { "OSC", "VCF", "ENV", "CHO", "PAT" };

// ------------------- Page slider builders -------------------
static void buildOscSliders() {
    PatchData& p = synth.patch();
    int x = 10;
    pageSliders[0] = { "SAW",   x, SLIDER_Y, SLIDER_W, SLIDER_H, &p.sawLevel,   0.0f, 1.0f }; x += SLIDER_W + 8;
    pageSliders[1] = { "PUL",   x, SLIDER_Y, SLIDER_W, SLIDER_H, &p.pulseLevel, 0.0f, 1.0f }; x += SLIDER_W + 8;
    pageSliders[2] = { "SUB",   x, SLIDER_Y, SLIDER_W, SLIDER_H, &p.subLevel,   0.0f, 1.0f }; x += SLIDER_W + 8;
    pageSliders[3] = { "PW",    x, SLIDER_Y, SLIDER_W, SLIDER_H, &p.pulseWidth, 0.05f, 0.95f }; x += SLIDER_W + 8;
    // LFO controls
    pageSliders[4] = { "LFO-R", x, SLIDER_Y, SLIDER_W, SLIDER_H, &p.lfoRate,    0.05f, 20.0f, true }; x += SLIDER_W + 8;
    pageSliders[5] = { "LFO-D", x, SLIDER_Y, SLIDER_W, SLIDER_H, &p.lfoDepth,   0.0f, 1.0f };
    pageSliderCount = 6;
}
static void buildVcfSliders() {
    PatchData& p = synth.patch();
    int x = 10;
    pageSliders[0] = { "HPF",   x, SLIDER_Y, SLIDER_W, SLIDER_H, &p.hpfCutoff, 20.0f, 1000.0f, true }; x += SLIDER_W + 8;
    pageSliders[1] = { "CUT",   x, SLIDER_Y, SLIDER_W, SLIDER_H, &p.cutoff,    40.0f, 8000.0f, true }; x += SLIDER_W + 8;
    pageSliders[2] = { "RES",   x, SLIDER_Y, SLIDER_W, SLIDER_H, &p.resonance, 0.7f,  5.0f };        x += SLIDER_W + 8;
    pageSliders[3] = { "ENV",   x, SLIDER_Y, SLIDER_W, SLIDER_H, &p.envAmount, 0.0f,  1.0f };
    pageSliderCount = 4;
}
static void buildEnvSliders() {
    PatchData& p = synth.patch();
    int x = 10;
    pageSliders[0] = { "A-A",   x, SLIDER_Y, SLIDER_W, SLIDER_H, &p.ampA, 0.0f, 3000.0f, true }; x += SLIDER_W + 4;
    pageSliders[1] = { "A-D",   x, SLIDER_Y, SLIDER_W, SLIDER_H, &p.ampD, 0.0f, 3000.0f, true }; x += SLIDER_W + 4;
    pageSliders[2] = { "A-S",   x, SLIDER_Y, SLIDER_W, SLIDER_H, &p.ampS, 0.0f, 1.0f };          x += SLIDER_W + 4;
    pageSliders[3] = { "A-R",   x, SLIDER_Y, SLIDER_W, SLIDER_H, &p.ampR, 0.0f, 5000.0f, true }; x += SLIDER_W + 16;
    pageSliders[4] = { "F-A",   x, SLIDER_Y, SLIDER_W, SLIDER_H, &p.fltA, 0.0f, 3000.0f, true }; x += SLIDER_W + 4;
    pageSliders[5] = { "F-D",   x, SLIDER_Y, SLIDER_W, SLIDER_H, &p.fltD, 0.0f, 3000.0f, true }; x += SLIDER_W + 4;
    pageSliders[6] = { "F-S",   x, SLIDER_Y, SLIDER_W, SLIDER_H, &p.fltS, 0.0f, 1.0f };          x += SLIDER_W + 4;
    pageSliders[7] = { "F-R",   x, SLIDER_Y, SLIDER_W, SLIDER_H, &p.fltR, 0.0f, 5000.0f, true };
    pageSliderCount = 8;
}
static void buildChorusSliders() {
    // Chorus page is mostly buttons, no continuous sliders.
    pageSliderCount = 0;
}
static void buildPageSliders(UIPage page) {
    switch (page) {
        case PAGE_OSC:    buildOscSliders(); break;
        case PAGE_VCF:    buildVcfSliders(); break;
        case PAGE_ENV:    buildEnvSliders(); break;
        case PAGE_CHORUS: buildChorusSliders(); break;
        default:          pageSliderCount = 0; break;
    }
}

// ------------------- UI: header / tabs / cal button -------------------

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

    calBtnW = 48; calBtnH = 22;
    calBtnX = SCREEN_W - calBtnW - 4;
    calBtnY = 4;

    osKeyboard.begin(&tft, &ts);

    drawAll();
}

void UI::drawCalButton(bool pressed) {
    uint16_t bg = pressed ? ILI9341_ORANGE : ILI9341_DARKGREY;
    tft.fillRoundRect(calBtnX, calBtnY, calBtnW, calBtnH, 4, bg);
    tft.drawRoundRect(calBtnX, calBtnY, calBtnW, calBtnH, 4, ILI9341_WHITE);
    tft.setTextColor(ILI9341_WHITE);
    tft.setTextSize(2);
    int tw = 3 * 6 * 2;
    tft.setCursor(calBtnX + (calBtnW - tw)/2, calBtnY + (calBtnH - 14)/2);
    tft.print("CAL");
}

void UI::drawHeader() {
    tft.fillRect(0, 0, SCREEN_W, HEADER_H, ILI9341_NAVY);
    tft.setTextColor(ILI9341_WHITE);
    tft.setTextSize(2);
    tft.setCursor(6, 7);
    tft.print("JUNO-106  ");
    tft.print(synth.patch().name);
    drawCalButton(false);
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
        tft.setTextSize(2);
        int tw = strlen(pageNames[i]) * 12;
        tft.setCursor(x + (tabW - tw)/2, TABS_Y + 5);
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

// ------------------- Body / pages -------------------

static void drawSliderAt(const Slider& s) {
    tft.drawRect(s.x, s.y, s.w, s.h, ILI9341_DARKGREY);
    tft.fillRect(s.x+1, s.y+1, s.w-2, s.h-2, ILI9341_BLACK);
    float v = *s.value;
    float norm;
    if (s.logarithmic && s.min > 0.0f && s.max > 0.0f) {
        norm = (logf(v / s.min) / logf(s.max / s.min));
    } else {
        norm = (v - s.min) / (s.max - s.min);
    }
    if (norm < 0) norm = 0; 
    if (norm > 1) norm = 1;
    int fillH = (int)((s.h - 2) * norm);
    tft.fillRect(s.x+1, s.y + s.h - 1 - fillH, s.w-2, fillH, ILI9341_ORANGE);
    tft.setTextColor(ILI9341_WHITE);
    tft.setTextSize(1);
    tft.setCursor(s.x + 2, s.y + s.h + 4);
    tft.print(s.label);
}

void UI::drawOscPage() {
    tft.fillRect(0, BODY_Y, SCREEN_W, BODY_H, ILI9341_BLACK);
    for (int i = 0; i < pageSliderCount; i++) drawSliderAt(pageSliders[i]);

    // LFO destination selector (right side)
    PatchData& p = synth.patch();
    const char* dests[] = {"OFF","PITCH","PW","FILT"};
    int bx = SCREEN_W - 90, by = SLIDER_Y, bw = 80, bh = 26;
    tft.setTextSize(1);
    tft.setTextColor(ILI9341_WHITE);
    tft.setCursor(bx, by - 10); tft.print("LFO DEST");
    for (int i = 0; i < 4; i++) {
        uint16_t bg = (p.lfoDest == i) ? ILI9341_ORANGE : ILI9341_DARKGREY;
        int y = by + i*(bh+2);
        tft.fillRoundRect(bx, y, bw, bh, 4, bg);
        tft.drawRoundRect(bx, y, bw, bh, 4, ILI9341_WHITE);
        tft.setTextColor(ILI9341_WHITE);
        tft.setTextSize(2);
        tft.setCursor(bx+8, y+6);
        tft.print(dests[i]);
    }
}

void UI::drawVcfPage() {
    tft.fillRect(0, BODY_Y, SCREEN_W, BODY_H, ILI9341_BLACK);
    for (int i = 0; i < pageSliderCount; i++) drawSliderAt(pageSliders[i]);
}
void UI::drawEnvPage() {
    tft.fillRect(0, BODY_Y, SCREEN_W, BODY_H, ILI9341_BLACK);
    // Section labels
    tft.setTextColor(ILI9341_CYAN);
    tft.setTextSize(1);
    tft.setCursor(14, BODY_Y + 2); tft.print("AMP ENV");
    tft.setCursor(180, BODY_Y + 2); tft.print("FILTER ENV");
    for (int i = 0; i < pageSliderCount; i++) drawSliderAt(pageSliders[i]);
}
void UI::drawChorusPage() {
    tft.fillRect(0, BODY_Y, SCREEN_W, BODY_H, ILI9341_BLACK);
    const char* modes[] = { "OFF", "CHORUS I", "CHORUS II" };
    uint8_t cur = synth.patch().chorusMode;
    int bw = 200, bh = 40, bx = (SCREEN_W - bw)/2;
    int by = BODY_Y + 20;
    tft.setTextSize(2);
    for (int i = 0; i < 3; i++) {
        uint16_t bg = (cur == i) ? ILI9341_ORANGE : ILI9341_DARKGREY;
        int y = by + i*(bh + 12);
        tft.fillRoundRect(bx, y, bw, bh, 6, bg);
        tft.drawRoundRect(bx, y, bw, bh, 6, ILI9341_WHITE);
        tft.setTextColor(ILI9341_WHITE);
        int tw = strlen(modes[i]) * 12;
        tft.setCursor(bx + (bw - tw)/2, y + (bh - 14)/2);
        tft.print(modes[i]);
    }
}

// --- Patch page (save/load) ---

static const int PATCH_COLS = 4;
static const int PATCH_ROWS = NUM_PATCH_SLOTS / PATCH_COLS;

static void patchSlotRect(int idx, int& x, int& y, int& w, int& h) {
    int col = idx % PATCH_COLS;
    int row = idx / PATCH_COLS;
    int cellW = (SCREEN_W - 110) / PATCH_COLS;
    int cellH = (BODY_H - 50) / PATCH_ROWS;
    x = 4 + col * cellW;
    y = BODY_Y + 4 + row * cellH;
    w = cellW - 4;
    h = cellH - 4;
}

void UI::drawPatchPage() {
    tft.fillRect(0, BODY_Y, SCREEN_W, BODY_H, ILI9341_BLACK);
    tft.setTextColor(ILI9341_CYAN);
    tft.setTextSize(1);
    tft.setCursor(4, BODY_Y + 2);
    tft.print("PATCHES (tap to select)");

    char nm[17];
    for (int i = 0; i < NUM_PATCH_SLOTS; i++) {
        int x,y,w,h; patchSlotRect(i, x, y, w, h);
        bool has = patchManager.getPatchName(i, nm);
        bool sel = (i == selectedSlot);
        uint16_t bg = sel ? ILI9341_ORANGE : (has ? ILI9341_NAVY : ILI9341_DARKGREY);
        tft.fillRoundRect(x, y, w, h, 4, bg);
        tft.drawRoundRect(x, y, w, h, 4, ILI9341_WHITE);
        tft.setTextColor(ILI9341_WHITE);
        tft.setTextSize(1);
        tft.setCursor(x + 3, y + 3);
        tft.printf("%02d", i);
        tft.setCursor(x + 3, y + 14);
        tft.print(has ? nm : "(empty)");
    }

    // Action buttons on right
    int bw = 96, bh = 36;
    int bx = SCREEN_W - bw - 6;
    int by1 = BODY_Y + 20;
    int by2 = by1 + bh + 10;
    int by3 = by2 + bh + 10;

    auto drawBtn = [&](int y, const char* label, uint16_t bg) {
        tft.fillRoundRect(bx, y, bw, bh, 6, bg);
        tft.drawRoundRect(bx, y, bw, bh, 6, ILI9341_WHITE);
        tft.setTextColor(ILI9341_WHITE);
        tft.setTextSize(2);
        int tw = strlen(label) * 12;
        tft.setCursor(bx + (bw - tw)/2, y + (bh - 14)/2);
        tft.print(label);
    };
    drawBtn(by1, "LOAD", ILI9341_DARKGREEN);
    drawBtn(by2, "SAVE", ILI9341_MAROON);
    drawBtn(by3, "INIT", ILI9341_DARKGREY);
}

int UI::patchSlotHitTest(int px, int py) const {
    for (int i = 0; i < NUM_PATCH_SLOTS; i++) {
        int x,y,w,h; patchSlotRect(i, x, y, w, h);
        if (px >= x && px <= x + w && py >= y && py <= y + h) return i;
    }
    return -1;
}

void UI::showStatus(const char* msg, uint16_t color) {
    int w = SCREEN_W - 20, h = 40;
    int x = 10, y = SCREEN_H/2 - h/2;
    tft.fillRoundRect(x, y, w, h, 6, color);
    tft.drawRoundRect(x, y, w, h, 6, ILI9341_WHITE);
    tft.setTextColor(ILI9341_WHITE);
    tft.setTextSize(2);
    int tw = strlen(msg) * 12;
    tft.setCursor(x + (w - tw)/2, y + (h - 14)/2);
    tft.print(msg);
    delay(800);
    drawBody();
}

void UI::drawBody() {
    buildPageSliders(currentPage);
    switch (currentPage) {
        case PAGE_OSC:    drawOscPage();    break;
        case PAGE_VCF:    drawVcfPage();    break;
        case PAGE_ENV:    drawEnvPage();    break;
        case PAGE_CHORUS: drawChorusPage(); break;
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

// ------------------- Input handling -------------------

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
    const int boxW = 240, boxH = 120;
    const int bx = (SCREEN_W - boxW) / 2;
    const int by = (SCREEN_H - boxH) / 2;
    tft.fillRoundRect(bx, by, boxW, boxH, 8, ILI9341_NAVY);
    tft.drawRoundRect(bx, by, boxW, boxH, 8, ILI9341_WHITE);
    tft.setTextColor(ILI9341_WHITE);
    tft.setTextSize(2);
    tft.setCursor(bx + 14, by + 14); tft.print("Recalibrate touch?");
    tft.setTextSize(1);
    tft.setCursor(bx + 14, by + 42); tft.print("You'll tap 3 corner crosshairs.");
    int btnW = 80, btnH = 34;
    int yesX = bx + 20, yesY = by + boxH - btnH - 14;
    int noX  = bx + boxW - btnW - 20, noY = yesY;
    tft.fillRoundRect(yesX, yesY, btnW, btnH, 6, ILI9341_DARKGREEN);
    tft.drawRoundRect(yesX, yesY, btnW, btnH, 6, ILI9341_WHITE);
    tft.setTextSize(2); tft.setTextColor(ILI9341_WHITE);
    tft.setCursor(yesX + 20, yesY + 9); tft.print("YES");
    tft.fillRoundRect(noX, noY, btnW, btnH, 6, ILI9341_MAROON);
    tft.drawRoundRect(noX, noY, btnW, btnH, 6, ILI9341_WHITE);
    tft.setCursor(noX + 26, noY + 9); tft.print("NO");

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
    // 1) CAL button
    if (inCalButton(x, y)) {
        drawCalButton(true);
        if (confirmRecalibrate()) recalibrateTouch();
        else drawAll();
        return;
    }
    // 2) Tabs
    int t = tabHitTest(x, y);
    if (t >= 0) {
        if ((UIPage)t != currentPage) {
            currentPage = (UIPage)t;
            drawTabs();
            drawBody();
        }
        return;
    }
    // 3) Sliders on current page
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
        synth.applyPatch(synth.patch());
        drawSliderAt(s);
        return;
    }
    // 4) Page-specific widgets
    if (currentPage == PAGE_OSC) {
        // LFO dest buttons
        int bx = SCREEN_W - 90, by = SLIDER_Y, bw = 80, bh = 26;
        if (x >= bx && x <= bx + bw) {
            for (int i = 0; i < 4; i++) {
                int yy = by + i*(bh+2);
                if (y >= yy && y <= yy + bh) {
                    synth.patch().lfoDest = (uint8_t)i;
                    synth.applyPatch(synth.patch());
                    drawOscPage();
                    return;
                }
            }
        }
    } else if (currentPage == PAGE_CHORUS) {
        int bw = 200, bh = 40, bx = (SCREEN_W - bw)/2;
        int by = BODY_Y + 20;
        for (int i = 0; i < 3; i++) {
            int yy = by + i*(bh+12);
            if (x >= bx && x <= bx + bw && y >= yy && y <= yy + bh) {
                synth.patch().chorusMode = (uint8_t)i;
                synth.applyPatch(synth.patch());
                drawChorusPage();
                return;
            }
        }
    } else if (currentPage == PAGE_PATCH) {
        // Slot hit?
        int slot = patchSlotHitTest(x, y);
        if (slot >= 0) { onPatchSlotTap(slot); return; }
        // Action buttons
        int bw = 96, bh = 36;
        int bx = SCREEN_W - bw - 6;
        int by1 = BODY_Y + 20, by2 = by1 + bh + 10, by3 = by2 + bh + 10;
        if (x >= bx && x <= bx + bw) {
            if (y >= by1 && y <= by1 + bh) {
                PatchData p;
                if (patchManager.loadPatch(selectedSlot, p)) {
                    synth.applyPatch(p);
                    drawHeader();   // patch name changed
                    showStatus("LOADED", ILI9341_DARKGREEN);
                } else {
                    showStatus("EMPTY SLOT", ILI9341_MAROON);
                }
                return;
            }
            if (y >= by2 && y <= by2 + bh) {
                // Ask for a name via the on-screen keyboard
                PatchData& p = synth.patch();
                char seed[17];
                if (p.name[0] == 0 || strcmp(p.name, "INIT PATCH") == 0) {
                    snprintf(seed, sizeof(seed), "SLOT %02d", selectedSlot);
                } else {
                    strncpy(seed, p.name, 16); seed[16] = 0;
                }
                char newName[17];
                bool ok = osKeyboard.edit("Name patch:", seed, newName, sizeof(newName));
                if (!ok) {
                    drawAll();  // user cancelled
                    return;
                }
                // Trim trailing spaces
                for (int i = strlen(newName) - 1; i >= 0; i--) {
                    if (newName[i] == ' ') newName[i] = 0; else break;
                }
                if (newName[0] == 0) strcpy(newName, "UNTITLED");
                strncpy(p.name, newName, sizeof(p.name) - 1);
                p.name[sizeof(p.name) - 1] = 0;

                bool saved = patchManager.savePatch(selectedSlot, p);
                drawAll();     // redraw full UI (was overwritten by keyboard)
                showStatus(saved ? "SAVED" : "SAVE FAIL",
                        saved ? ILI9341_DARKGREEN : ILI9341_MAROON);
                return;
            }
            if (y >= by3 && y <= by3 + bh) {
                PatchData init;
                synth.applyPatch(init);
                drawHeader();
                drawBody();
                showStatus("INIT PATCH", ILI9341_NAVY);
                return;
            }
        }
    }
}

void UI::update() {
    if (ts.touched()) {
        TS_Point p = ts.getPoint();
        int16_t sx, sy;
        touchCal.mapToScreen(p.x, p.y, sx, sy);
        handleTouch(sx, sy);
        // Small debounce so slider doesn't jitter between reads
        delay(15);
    }
    // Periodic header refresh (patch name could change)
    if (millis() - lastNames > 500) {
        lastNames = millis();
        drawHeader();
    }
}
