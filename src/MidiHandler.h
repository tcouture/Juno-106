#pragma once
#include <Arduino.h>
#include <MIDI.h>

class MidiHandler {
public:
    void begin();
    void update();
};

extern MidiHandler midiHandler;

