#include "UsbHostMidi.h"
#include "MidiDispatch.h"

// USB host stack
static USBHost              usbHost;
static USBHub               hub1(usbHost);
static USBHub               hub2(usbHost);
static MIDIDevice_BigBuffer hostMIDI(usbHost);

UsbHostMidi hostMidi;

void UsbHostMidi::onNoteOn(byte ch, byte note, byte vel) {
    midiDispatchNoteOn(ch, note, vel, MidiSource::UsbHost);
}

void UsbHostMidi::onNoteOff(byte ch, byte note, byte vel) {
    midiDispatchNoteOff(ch, note, vel, MidiSource::UsbHost);
}

void UsbHostMidi::onCC(byte ch, byte cc, byte val) {
    midiDispatchCC(ch, cc, val, MidiSource::UsbHost);
}

void UsbHostMidi::onPitchBend(byte ch, int bend) {
    midiDispatchPitchBend(ch, bend, MidiSource::UsbHost);
}

void UsbHostMidi::begin() {
    usbHost.begin();

    hostMIDI.setHandleNoteOn(onNoteOn);
    hostMIDI.setHandleNoteOff(onNoteOff);
    hostMIDI.setHandleControlChange(onCC);
    hostMIDI.setHandlePitchChange(onPitchBend);

    Serial.println("USB Host MIDI started. Plug in a keyboard.");
}

void UsbHostMidi::update() {
    usbHost.Task();
    hostMIDI.read();
}
