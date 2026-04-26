#include "MidiHandler.h"
#include "SynthEngine.h"
#include "Config.h"

MIDI_CREATE_INSTANCE(HardwareSerial, MIDI_SERIAL, dinMidi);
MidiHandler midiHandler;

static void handleNoteOnDin(byte ch, byte note, byte vel) {
    synth.noteOn(note, vel);
}
static void handleNoteOffDin(byte ch, byte note, byte vel) {
    synth.noteOff(note);
}
static void handleCCDin(byte ch, byte cc, byte val) {
    // Hook CCs here (e.g., #74 = cutoff, #71 = resonance)
    PatchData& p = synth.patch();
    switch (cc) {
        case 74: p.cutoff   = 40.0f + (val / 127.0f) * 8000.0f; break;
        case 71: p.resonance= 0.7f  + (val / 127.0f) * 4.3f;    break;
        case 123: synth.allNotesOff(); return;
        default: return;
    }
    synth.applyPatch(p);
}

void MidiHandler::begin() {
    dinMidi.begin(MIDI_CHANNEL_OMNI);
    dinMidi.setHandleNoteOn(handleNoteOnDin);
    dinMidi.setHandleNoteOff(handleNoteOffDin);
    dinMidi.setHandleControlChange(handleCCDin);

    usbMIDI.setHandleNoteOn([](byte ch, byte n, byte v){ synth.noteOn(n, v); });
    usbMIDI.setHandleNoteOff([](byte ch, byte n, byte v){ synth.noteOff(n); });
    usbMIDI.setHandleControlChange([](byte ch, byte cc, byte v){
        handleCCDin(ch, cc, v);
    });
}

void MidiHandler::update() {
    dinMidi.read();
    while (usbMIDI.read()) {}
}

