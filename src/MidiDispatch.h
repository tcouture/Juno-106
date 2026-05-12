#pragma once
#include <Arduino.h>
#include "MidiActivity.h"

void midiDispatchNoteOn(byte ch, byte note, byte vel, MidiSource src);
void midiDispatchNoteOff(byte ch, byte note, byte vel, MidiSource src);
void midiDispatchCC(byte ch, byte cc, byte val, MidiSource src);
void midiDispatchPitchBend(byte ch, int bend, MidiSource src);
