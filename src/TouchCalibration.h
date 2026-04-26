#pragma once
#include <Arduino.h>
#include <XPT2046_Touchscreen.h>
#include <ILI9341_t3.h>

struct TouchCalData {
    uint32_t magic;     // validity marker
    int16_t  xMin;
    int16_t  xMax;
    int16_t  yMin;
    int16_t  yMax;
    uint8_t  swapXY;    // 0 or 1
    uint8_t  invertX;   // 0 or 1
    uint8_t  invertY;   // 0 or 1
};

class TouchCalibration {
public:
    static constexpr uint32_t MAGIC = 0xCA11B8A7;

    void begin(ILI9341_t3* tft, XPT2046_Touchscreen* ts);

    // Run the on-screen calibration wizard (blocking).
    bool runWizard();

    // Convert raw touch coords to screen pixel coords using current cal data.
    void mapToScreen(int16_t rawX, int16_t rawY, int16_t& outX, int16_t& outY);

    // Persistence
    bool loadFromSD();
    bool saveToSD();

    const TouchCalData& data() const { return cal; }
    bool isValid() const { return cal.magic == MAGIC; }

private:
    ILI9341_t3*            tft_ = nullptr;
    XPT2046_Touchscreen*   ts_  = nullptr;
    TouchCalData cal = {};

    bool captureTarget(int screenX, int screenY, int16_t& rawX, int16_t& rawY);
    void drawCrosshair(int x, int y, uint16_t color);
    void waitForRelease();
    bool waitForPress(int16_t& rawX, int16_t& rawY, uint32_t timeoutMs = 30000);
};

extern TouchCalibration touchCal;

