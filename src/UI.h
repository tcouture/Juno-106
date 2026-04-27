#pragma once
#include <Arduino.h>

enum UIPage : uint8_t {
    PAGE_PATCH = 0,
    PAGE_OSC,
    PAGE_VCF,
    PAGE_ENV,
    PAGE_CHORUS,
    PAGE_PERF,
    PAGE_COUNT
};

class UI {
public:
    void begin();
    void update();
    void drawAll();
    void recalibrateTouch();

    // Let main.cpp indicate which slot was loaded at boot so the UI
    // can highlight it with the "loaded" border.
    void setLoadedSlot(int slot) { loadedSlot = slot; }

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

    UIPage  currentPage = PAGE_PATCH;

    int16_t calBtnX = 0, calBtnY = 0, calBtnW = 0, calBtnH = 0;

    int selectedSlot = 0;
    int loadedSlot   = -1;          // -1 = no stored slot is currently loaded
    uint32_t lastNames = 0;

    // Header layout (recomputed in begin() so it scales with MAX_VOICES)
    int16_t hdrCpuX = 0, hdrCpuY = 0, hdrCpuW = 0;
    int16_t hdrDotsX = 0, hdrDotsY = 0;
    int16_t hdrDotRadius = 0, hdrDotPitch = 0;
    int16_t hdrNameX = 0;
    uint8_t hdrDotsPerRow = 0;

    void computeHeaderLayout();
    void drawHeaderVoiceDots();
    void drawHeaderCpu();

    // Meter layout
    int16_t meterX = 0, meterY = 0, meterW = 0, meterH = 0;
    float   meterPeakL = 0.0f, meterPeakR = 0.0f;
    float   meterHoldL = 0.0f, meterHoldR = 0.0f;
    uint32_t meterHoldMsL = 0, meterHoldMsR = 0;

    void drawHeaderMeter();

    // MIDI activity indicators
    int16_t midiActX = 0, midiActY = 0;
    void drawMidiActivity();
};

extern UI ui;
