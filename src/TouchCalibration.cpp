#include "TouchCalibration.h"
#include "Config.h"
#include <SD.h>
#include <SPI.h>

TouchCalibration touchCal;

static const char* CAL_FILE = "/patches/touchcal.bin";

void TouchCalibration::begin(ILI9341_t3* tft, XPT2046_Touchscreen* ts) {
    tft_ = tft;
    ts_  = ts;
    // Use rotation 0 internally during calibration so raw coords are predictable,
    // then store calibration that includes axis swap/invert to match DISPLAY_ROTATION.
}

void TouchCalibration::drawCrosshair(int x, int y, uint16_t color) {
    tft_->drawFastHLine(x - 10, y, 21, color);
    tft_->drawFastVLine(x, y - 10, 21, color);
    tft_->drawCircle(x, y, 6, color);
}

void TouchCalibration::waitForRelease() {
    while (ts_->touched()) { delay(10); }
    delay(50);
}

bool TouchCalibration::waitForPress(int16_t& rawX, int16_t& rawY, uint32_t timeoutMs) {
    uint32_t start = millis();
    // Wait for touch
    while (!ts_->touched()) {
        if (millis() - start > timeoutMs) return false;
        delay(5);
    }
    // Debounce / average a few samples
    int32_t sx = 0, sy = 0;
    const int N = 16;
    for (int i = 0; i < N; i++) {
        TS_Point p = ts_->getPoint();
        sx += p.x;
        sy += p.y;
        delay(5);
    }
    rawX = sx / N;
    rawY = sy / N;
    waitForRelease();
    return true;
}

bool TouchCalibration::captureTarget(int screenX, int screenY,
                                     int16_t& rawX, int16_t& rawY) {
    tft_->fillScreen(ILI9341_BLACK);
    tft_->setTextColor(ILI9341_WHITE);
    tft_->setTextSize(2);
    tft_->setCursor(20, 20);
    tft_->print("Touch the crosshair");
    drawCrosshair(screenX, screenY, ILI9341_YELLOW);
    return waitForPress(rawX, rawY);
}

bool TouchCalibration::runWizard() {
    if (!tft_ || !ts_) return false;

    // During the wizard we want the display in the user's chosen orientation.
    tft_->setRotation(DISPLAY_ROTATION);
    ts_->setRotation(0);   // read RAW, unrotated

    const int W = tft_->width();
    const int H = tft_->height();

    // Three targets: top-left, top-right, bottom-left
    const int inset = 20;
    int16_t rx_tl, ry_tl, rx_tr, ry_tr, rx_bl, ry_bl;

    tft_->fillScreen(ILI9341_BLACK);
    tft_->setTextColor(ILI9341_CYAN);
    tft_->setTextSize(2);
    tft_->setCursor(20, H/2 - 20);
    tft_->print("Touch Calibration");
    tft_->setTextSize(1);
    tft_->setCursor(20, H/2 + 10);
    tft_->print("Tap each crosshair accurately.");
    delay(1500);

    if (!captureTarget(inset,       inset,     rx_tl, ry_tl)) return false;
    if (!captureTarget(W - inset-1, inset,     rx_tr, ry_tr)) return false;
    if (!captureTarget(inset,       H - inset-1, rx_bl, ry_bl)) return false;

    // Decide axis mapping: which raw axis varies most between TL->TR (horizontal move)?
    int dxRawX = abs(rx_tr - rx_tl);
    int dxRawY = abs(ry_tr - ry_tl);
    bool swapXY = (dxRawY > dxRawX);

    int16_t rawXLeft, rawXRight, rawYTop, rawYBottom;
    if (!swapXY) {
        rawXLeft   = rx_tl;
        rawXRight  = rx_tr;
        rawYTop    = ry_tl;
        rawYBottom = ry_bl;
    } else {
        // Touch axes are swapped relative to display
        rawXLeft   = ry_tl;
        rawXRight  = ry_tr;
        rawYTop    = rx_tl;
        rawYBottom = rx_bl;
    }

    bool invertX = (rawXLeft > rawXRight);
    bool invertY = (rawYTop  > rawYBottom);

    int16_t xMin = min(rawXLeft, rawXRight);
    int16_t xMax = max(rawXLeft, rawXRight);
    int16_t yMin = min(rawYTop,  rawYBottom);
    int16_t yMax = max(rawYTop,  rawYBottom);

    // Extrapolate from the inset to the true edges (0 and W-1 / H-1)
    float pxPerRawX = (float)(W - 2*inset) / (float)(xMax - xMin);
    float pxPerRawY = (float)(H - 2*inset) / (float)(yMax - yMin);
    int16_t extendX = (int16_t)(inset / pxPerRawX);
    int16_t extendY = (int16_t)(inset / pxPerRawY);
    xMin -= extendX; xMax += extendX;
    yMin -= extendY; yMax += extendY;

    cal.magic   = MAGIC;
    cal.xMin    = xMin;
    cal.xMax    = xMax;
    cal.yMin    = yMin;
    cal.yMax    = yMax;
    cal.swapXY  = swapXY ? 1 : 0;
    cal.invertX = invertX ? 1 : 0;
    cal.invertY = invertY ? 1 : 0;

    // Show result
    tft_->fillScreen(ILI9341_BLACK);
    tft_->setTextColor(ILI9341_GREEN);
    tft_->setTextSize(2);
    tft_->setCursor(20, 20);
    tft_->print("Calibration done");
    tft_->setTextSize(1);
    tft_->setTextColor(ILI9341_WHITE);
    tft_->setCursor(20, 60);
    tft_->printf("xMin=%d xMax=%d", cal.xMin, cal.xMax);
    tft_->setCursor(20, 75);
    tft_->printf("yMin=%d yMax=%d", cal.yMin, cal.yMax);
    tft_->setCursor(20, 90);
    tft_->printf("swapXY=%d invX=%d invY=%d", cal.swapXY, cal.invertX, cal.invertY);
    tft_->setCursor(20, 120);
    tft_->print("Tap to verify, or wait 3s...");

    // Quick verification: show crosshair wherever user taps, for ~3 seconds idle
    uint32_t t0 = millis();
    while (millis() - t0 < 3000) {
        if (ts_->touched()) {
            TS_Point p = ts_->getPoint();
            int16_t sx, sy;
            mapToScreen(p.x, p.y, sx, sy);
            drawCrosshair(sx, sy, ILI9341_ORANGE);
            t0 = millis(); // keep extending while user taps
            waitForRelease();
        }
        delay(10);
    }

    bool saved = saveToSD();
    tft_->fillScreen(ILI9341_BLACK);
    tft_->setTextColor(saved ? ILI9341_GREEN : ILI9341_RED);
    tft_->setTextSize(2);
    tft_->setCursor(20, 100);
    tft_->print(saved ? "Saved to SD" : "SD save failed");
    delay(1200);

    // Restore user's chosen rotations (caller typically re-sets them too)
    ts_->setRotation(0);
    return true;
}

void TouchCalibration::mapToScreen(int16_t rawX, int16_t rawY,
                                   int16_t& outX, int16_t& outY) {
    if (!tft_) { outX = 0; outY = 0; return; }
    int16_t rx = cal.swapXY ? rawY : rawX;
    int16_t ry = cal.swapXY ? rawX : rawY;

    int W = tft_->width();
    int H = tft_->height();

    int16_t sx = map(rx, cal.xMin, cal.xMax, 0, W - 1);
    int16_t sy = map(ry, cal.yMin, cal.yMax, 0, H - 1);

    if (cal.invertX) sx = (W - 1) - sx;
    if (cal.invertY) sy = (H - 1) - sy;

    if (sx < 0) sx = 0; 
    if (sx >= W) sx = W - 1;
    if (sy < 0) sy = 0; 
    if (sy >= H) sy = H - 1;

    outX = sx;
    outY = sy;
}

bool TouchCalibration::loadFromSD() {
    // Assumes SD has already been initialized by PatchManager::begin().
    if (!SD.exists(CAL_FILE)) return false;
    File f = SD.open(CAL_FILE, FILE_READ);
    if (!f) return false;
    TouchCalData tmp;
    size_t n = f.read((uint8_t*)&tmp, sizeof(tmp));
    f.close();
    if (n != sizeof(tmp) || tmp.magic != MAGIC) return false;
    cal = tmp;
    return true;
}

bool TouchCalibration::saveToSD() {
    if (SD.exists(CAL_FILE)) SD.remove(CAL_FILE);
    File f = SD.open(CAL_FILE, FILE_WRITE);
    if (!f) return false;
    f.write((const uint8_t*)&cal, sizeof(cal));
    f.close();
    return true;
}

