#include "OnScreenKeyboard.h"
#include "Config.h"
#include "TouchCalibration.h"

OnScreenKeyboard osKeyboard;

static const char* ROW1_LOWER = "1234567890";
static const char* ROW2_LOWER = "qwertyuiop";
static const char* ROW3_LOWER = "asdfghjkl";
static const char* ROW4_LOWER = "zxcvbnm";

static const char* ROW1_UPPER = "!@#$%^&*()";
static const char* ROW2_UPPER = "QWERTYUIOP";
static const char* ROW3_UPPER = "ASDFGHJKL";
static const char* ROW4_UPPER = "ZXCVBNM";

void OnScreenKeyboard::begin(ILI9341_t3* tft, XPT2046_Touchscreen* ts) {
    tft_ = tft;
    ts_  = ts;
}

void OnScreenKeyboard::buildLayout() {
    keyCount = 0;

    const int startY = 70;
    const int keyH   = 30;
    const int gap    = 3;

    auto addRow = [&](const char* lower, const char* upper, int y, int startX) {
        const char* row = shift ? upper : lower;
        int n = strlen(row);
        int keyW = 28;
        int x = startX;
        for (int i = 0; i < n; i++) {
            if (keyCount >= MAX_KEYS) return;
            Key& k = keys[keyCount++];
            k.label = nullptr;
            k.ch    = row[i];
            k.x = x; k.y = y; k.w = keyW; k.h = keyH;
            k.action = 0;
            x += keyW + gap;
        }
    };

    addRow(ROW1_LOWER, ROW1_UPPER, startY,              10);
    addRow(ROW2_LOWER, ROW2_UPPER, startY + 1*(keyH+gap), 10);
    addRow(ROW3_LOWER, ROW3_UPPER, startY + 2*(keyH+gap), 22);
    addRow(ROW4_LOWER, ROW4_UPPER, startY + 3*(keyH+gap), 40);

    // Bottom row: SHIFT | SPACE | BS | CANCEL | OK
    int y = startY + 4*(keyH+gap);

    auto addSpecial = [&](const char* label, uint8_t action, int x, int w) {
        if (keyCount >= MAX_KEYS) return;
        Key& k = keys[keyCount++];
        k.label = label;
        k.ch = 0;
        k.x = x; k.y = y; k.w = w; k.h = keyH;
        k.action = action;
    };

    addSpecial("SH",     2, 10, 40);
    addSpecial("SPACE",  3, 54, 120);
    addSpecial("BS",     1, 178, 38);
    addSpecial("CNCL",   5, 220, 44);
    addSpecial("OK",     4, 268, 44);
}

void OnScreenKeyboard::drawKeys() {
    for (int i = 0; i < keyCount; i++) {
        const Key& k = keys[i];
        uint16_t bg = ILI9341_DARKGREY;
        if (k.action == 4) bg = ILI9341_DARKGREEN;        // OK
        else if (k.action == 5) bg = ILI9341_MAROON;      // CANCEL
        else if (k.action == 2 && shift) bg = ILI9341_ORANGE;

        tft_->fillRoundRect(k.x, k.y, k.w, k.h, 3, bg);
        tft_->drawRoundRect(k.x, k.y, k.w, k.h, 3, ILI9341_WHITE);
        tft_->setTextColor(ILI9341_WHITE);
        tft_->setTextSize(2);
        if (k.action == 0) {
            int tw = 12;
            tft_->setCursor(k.x + (k.w - tw)/2, k.y + (k.h - 14)/2);
            tft_->print((char)k.ch);
        } else {
            int tw = strlen(k.label) * 12;
            tft_->setCursor(k.x + (k.w - tw)/2, k.y + (k.h - 14)/2);
            tft_->print(k.label);
        }
    }
}

void OnScreenKeyboard::drawBuffer() {
    int bx = 10, by = 40, bw = SCREEN_W - 20, bh = 24;
    tft_->fillRect(bx, by, bw, bh, ILI9341_BLACK);
    tft_->drawRect(bx, by, bw, bh, ILI9341_WHITE);
    tft_->setTextColor(ILI9341_YELLOW);
    tft_->setTextSize(2);
    tft_->setCursor(bx + 6, by + 4);
    tft_->print(buffer);
    // Caret
    int cx = bx + 6 + cursor * 12;
    tft_->drawFastVLine(cx, by + 3, bh - 6, ILI9341_CYAN);
}

void OnScreenKeyboard::drawAll(const char* title) {
    tft_->fillScreen(ILI9341_BLACK);
    tft_->setTextColor(ILI9341_CYAN);
    tft_->setTextSize(2);
    tft_->setCursor(10, 10);
    tft_->print(title);
    drawBuffer();
    drawKeys();
}

int OnScreenKeyboard::hitTestKey(int x, int y) const {
    for (int i = 0; i < keyCount; i++) {
        const Key& k = keys[i];
        if (x >= k.x && x <= k.x + k.w && y >= k.y && y <= k.y + k.h) return i;
    }
    return -1;
}

void OnScreenKeyboard::insertChar(char c) {
    if (cursor >= 16) return;
    buffer[cursor++] = c;
    buffer[cursor]   = 0;
}
void OnScreenKeyboard::backspace() {
    if (cursor == 0) return;
    buffer[--cursor] = 0;
}

bool OnScreenKeyboard::edit(const char* title, const char* initialText,
                            char* outText, size_t outTextSize) {
    if (!tft_ || !ts_) return false;
    if (outTextSize < 17) return false;

    memset(buffer, 0, sizeof(buffer));
    if (initialText) {
        strncpy(buffer, initialText, 16);
        buffer[16] = 0;
    }
    cursor = strlen(buffer);
    shift  = false;

    buildLayout();
    drawAll(title);

    // Drain any held touch so the tap that opened us isn't consumed.
    while (ts_->touched()) delay(10);
    delay(80);

    while (true) {
        if (ts_->touched()) {
            TS_Point p = ts_->getPointRaw();
            int16_t sx, sy;
            touchCal.mapToScreen(p.x, p.y, sx, sy);
            int idx = hitTestKey(sx, sy);
            // Wait for release (simple debounce)
            while (ts_->touched()) delay(8);
            if (idx < 0) continue;

            Key& k = keys[idx];
            switch (k.action) {
                case 0: insertChar(k.ch); drawBuffer(); break;
                case 1: backspace();      drawBuffer(); break;
                case 2: shift = !shift;   buildLayout(); drawKeys(); break;
                case 3: insertChar(' ');  drawBuffer(); break;
                case 4: // OK
                    strncpy(outText, buffer, outTextSize - 1);
                    outText[outTextSize - 1] = 0;
                    return true;
                case 5: // CANCEL
                    return false;
            }
        }
        delay(10);
    }
}

