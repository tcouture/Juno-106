#pragma once
#include <Arduino.h>

enum UIPage : uint8_t {
    PAGE_OSC = 0,
    PAGE_VCF,
    PAGE_ENV,
    PAGE_CHORUS,
    PAGE_PERF,
    PAGE_PATCH,
    PAGE_COUNT
};

class UI {
public:
    void begin();
    void update();
    void drawAll();
    void recalibrateTouch();

private:
    void drawHeader();
    void drawTabs();
    void drawBody();
    void drawCalButton(bool pressed = false);

    void drawOscPage();
    void drawVcfPage();
    void drawEnvPage();
    void drawChorusPage();
    void drawPerfPage();
    void drawPatchPage();

    void handleTouch(int x, int y);
    bool inCalButton(int x, int y) const;
    int  tabHitTest(int x, int y) const;
    int  sliderHitTest(int x, int y) const;

    bool confirmRecalibrate();
    void showStatus(const char* msg, uint16_t color);

    int  patchSlotHitTest(int x, int y) const;
    void onPatchSlotTap(int slotIdx);

    UIPage currentPage = PAGE_OSC;
    int16_t calBtnX = 0, calBtnY = 0, calBtnW = 0, calBtnH = 0;

    int selectedSlot = 0;
    uint32_t lastNames = 0;
};

extern UI ui;
