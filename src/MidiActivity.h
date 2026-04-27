#pragma once
#include <Arduino.h>

enum class MidiSource : uint8_t {
    UsbDevice = 0,    // Teensy's micro-USB (usbMIDI)
    UsbHost   = 1,    // Teensy's USB host port
    Din       = 2,    // Serial6 DIN MIDI
    Count
};

class MidiActivity {
public:
    void ping(MidiSource s) {
        if ((uint8_t)s < (uint8_t)MidiSource::Count) {
            lastMs[(uint8_t)s] = millis();
        }
    }

    // Returns a brightness factor 0..1 for a source, based on how
    // recently it was pinged. Decays over ~200 ms.
    float intensity(MidiSource s) const {
        uint8_t i = (uint8_t)s;
        if (i >= (uint8_t)MidiSource::Count) return 0.0f;
        uint32_t now = millis();
        uint32_t dt  = now - lastMs[i];
        if (dt >= decayMs) return 0.0f;
        return 1.0f - (float)dt / (float)decayMs;
    }

private:
    static constexpr uint32_t decayMs = 200;
    uint32_t lastMs[(size_t)MidiSource::Count] = {0};
};

extern MidiActivity midiActivity;

