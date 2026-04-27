#include "UsbHostMidi.h"
#include "SynthEngine.h"
#include "Arpeggiator.h"
#include "MidiActivity.h"

// USB host stack
static USBHost         usbHost;
static USBHub          hub1(usbHost);
static USBHub          hub2(usbHost);
static MIDIDevice_BigBuffer hostMIDI(usbHost);

UsbHostMidi hostMidi;

// Bend range must match MidiHandler's
static constexpr float PITCH_BEND_RANGE_SEMI = 2.0f;

void UsbHostMidi::onNoteOn(byte, byte note, byte vel) {
    midiActivity.ping(MidiSource::UsbHost);
    if (vel == 0) { arp.noteOff(note); return; }
    arp.noteOn(note, vel);
}

void UsbHostMidi::onNoteOff(byte, byte note, byte) {
    midiActivity.ping(MidiSource::UsbHost);
    arp.noteOff(note);
}

void UsbHostMidi::onCC(byte, byte cc, byte val) {
    midiActivity.ping(MidiSource::UsbHost);
    // ... existing body unchanged ...
}

void UsbHostMidi::onPitchBend(byte, int bend) {
    midiActivity.ping(MidiSource::UsbHost);
    float norm = (float)bend / 8192.0f;
    if (norm > 1.0f)  norm = 1.0f;
    if (norm < -1.0f) norm = -1.0f;
    synth.setPitchBend(norm * PITCH_BEND_RANGE_SEMI);
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

