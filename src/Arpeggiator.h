#pragma once
#include <Arduino.h>

enum ArpMode : uint8_t {
    ARP_OFF  = 0,
    ARP_UP   = 1,
    ARP_DOWN = 2,
    ARP_UPDN = 3,
    ARP_RND  = 4
};

class Arpeggiator {
public:
    void begin();
    void noteOn(uint8_t note, uint8_t velocity);
    void noteOff(uint8_t note);
    void tick(uint32_t nowMs);

    void setMode(ArpMode m);
    void setRateHz(float hz);
    void setOctaves(uint8_t o);
    void setGate(float g) { if (g<0.05f)g=0.05f; if(g>0.95f)g=0.95f; gate=g; }

    ArpMode getMode()    const { return mode; }
    float   getRateHz()  const { return rateHz; }
    uint8_t getOctaves() const { return octaves; }

private:
    static constexpr int MAX_HELD = 16;
    uint8_t held[MAX_HELD];
    uint8_t heldVel[MAX_HELD];
    int     heldCount = 0;

    ArpMode mode    = ARP_OFF;
    float   rateHz  = 4.0f;
    uint8_t octaves = 1;
    float   gate    = 0.5f;

    int  stepIdx = 0;
    int  octIdx  = 0;
    bool goingUp = true;

    uint8_t  lastPlayedNote = 255;
    uint32_t lastStepMs     = 0;
    uint32_t noteOffDueMs   = 0;

    void allOff();
    int  advanceStep();
};

extern Arpeggiator arp;
