#pragma once
#include <Arduino.h>
#include <USBHost_t36.h>

class UsbHostMidi {
public:
    void begin();
    void update();   // call from loop()

private:
    static void onNoteOn(byte channel, byte note, byte velocity);
    static void onNoteOff(byte channel, byte note, byte velocity);
    static void onCC(byte channel, byte cc, byte value);
    static void onPitchBend(byte channel, int bend);
};

extern UsbHostMidi hostMidi;

