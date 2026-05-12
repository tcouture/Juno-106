#include "MidiHandler.h"
#include "Config.h"
#include "MidiDispatch.h"

MIDI_CREATE_INSTANCE(HardwareSerial, MIDI_SERIAL, dinMidi);
MidiHandler midiHandler;

// ---- DIN wrappers ----
static void dinNoteOn (byte ch, byte n, byte v) { midiDispatchNoteOn(ch, n, v, MidiSource::Din); }
static void dinNoteOff(byte ch, byte n, byte v) { midiDispatchNoteOff(ch, n, v, MidiSource::Din); }
static void dinCC     (byte ch, byte c, byte v) { midiDispatchCC(ch, c, v, MidiSource::Din); }
static void dinBend   (byte ch, int  b)         { midiDispatchPitchBend(ch, b, MidiSource::Din); }

// ---- USB-device wrappers ----
static void usbNoteOn (byte ch, byte n, byte v) { midiDispatchNoteOn(ch, n, v, MidiSource::UsbDevice); }
static void usbNoteOff(byte ch, byte n, byte v) { midiDispatchNoteOff(ch, n, v, MidiSource::UsbDevice); }
static void usbCC     (byte ch, byte c, byte v) { midiDispatchCC(ch, c, v, MidiSource::UsbDevice); }
static void usbBend   (byte ch, int  b)         { midiDispatchPitchBend(ch, b, MidiSource::UsbDevice); }

void MidiHandler::begin() {
    dinMidi.begin(MIDI_CHANNEL_OMNI);
    dinMidi.setHandleNoteOn(dinNoteOn);
    dinMidi.setHandleNoteOff(dinNoteOff);
    dinMidi.setHandleControlChange(dinCC);
    dinMidi.setHandlePitchBend(dinBend);

    usbMIDI.setHandleNoteOn(usbNoteOn);
    usbMIDI.setHandleNoteOff(usbNoteOff);
    usbMIDI.setHandleControlChange(usbCC);
    usbMIDI.setHandlePitchChange(usbBend);
}

void MidiHandler::update() {
    dinMidi.read();
    while (usbMIDI.read()) {}
}
