#pragma once
#include <Arduino.h>
#include <ILI9341_t3.h>
#include <XPT2046_Touchscreen.h>

class OnScreenKeyboard {
public:
    void begin(ILI9341_t3* tft, XPT2046_Touchscreen* ts);

    // Show a modal keyboard. initialText is seeded into the edit buffer.
    // Returns true if user pressed OK, false if CANCEL. Final string in outText.
    bool edit(const char* title, const char* initialText,
              char* outText, size_t outTextSize /*>=17*/);

private:
    ILI9341_t3*          tft_ = nullptr;
    XPT2046_Touchscreen* ts_  = nullptr;

    char   buffer[17];
    int    cursor = 0;
    bool   shift  = false;

    void drawAll(const char* title);
    void drawBuffer();
    void drawKeys();

    struct Key {
        const char* label;   // display label
        char        ch;      // character to insert; 0 if special
        int16_t     x, y, w, h;
        uint8_t     action;  // 0=char, 1=backspace, 2=shift, 3=space, 4=ok, 5=cancel
    };
    static constexpr int MAX_KEYS = 64;
    Key keys[MAX_KEYS];
    int keyCount = 0;

    void buildLayout();
    int  hitTestKey(int x, int y) const;
    void pressKey(int i);
    void insertChar(char c);
    void backspace();
};

extern OnScreenKeyboard osKeyboard;

