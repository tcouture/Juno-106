#pragma once
#include <Arduino.h>

enum UIPage : uint8_t {
    PAGE_OSC = 0,
    PAGE_VCF,
    PAGE_ENV,
    PAGE_CHORUS,
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
    // layout
    void drawHeader();
    void drawTabs();
    void drawBody();
    void drawCalButton(bool pressed = false);

    // page drawers
    void drawOscPage();
    void drawVcfPage();
    void drawEnvPage();
    void drawChorusPage();
    void drawPatchPage();

    // interaction
    void handleTouch(int x, int y);
    bool inCalButton(int x, int y) const;
    int  tabHitTest(int x, int y) const;     // returns page index or -1
    int  sliderHitTest(int x, int y) const;  // returns index into current page's sliders or -1

    bool confirmRecalibrate();
    void showStatus(const char* msg, uint16_t color);

    // patch page helpers
    int  patchSlotHitTest(int x, int y) const;
    void onPatchSlotTap(int slotIdx);

    UIPage currentPage = PAGE_OSC;

    // CAL button
    int16_t calBtnX = 0, calBtnY = 0, calBtnW = 0, calBtnH = 0;

    // Patch page state
    int selectedSlot = 0;
    uint32_t lastNames = 0;
};

extern UI ui;
