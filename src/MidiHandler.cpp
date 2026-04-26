#include "MidiHandler.h"
#include "SynthEngine.h"
#include "Config.h"

MIDI_CREATE_INSTANCE(HardwareSerial, MIDI_SERIAL, dinMidi);
MidiHandler midiHandler;

// ---- Bend range in semitones (±) ----
static constexpr float PITCH_BEND_RANGE_SEMI = 2.0f;

// ---- Shared note/CC/bend handlers (DIN + USB) ----
static void onNoteOn(byte /*ch*/, byte note, byte vel) {
    synth.noteOn(note, vel);
}
static void onNoteOff(byte /*ch*/, byte note, byte /*vel*/) {
    synth.noteOff(note);
}
static void onCC(byte /*ch*/, byte cc, byte val) {
    PatchData& p = synth.patch();
    float vf = val / 127.0f;

    switch (cc) {
        case 1:   // Mod wheel
            synth.setModWheel(vf);
            return;

        // Oscillator mix
        case 20:  p.sawLevel   = vf; break;
        case 21:  p.pulseLevel = vf; break;
        case 22:  p.subLevel   = vf; break;
        case 70:  p.pulseWidth = 0.05f + vf * 0.9f; break;

        // Filter
        case 74:  p.cutoff    = 40.0f + vf * 7960.0f; break;
        case 71:  p.resonance = 0.7f  + vf * 4.3f;    break;
        case 79:  p.envAmount = vf;                   break;
        case 75:  p.hpfCutoff = 20.0f + vf * 980.0f;  break;

        // Amp envelope
        case 73:  p.ampA = vf * 3000.0f; break;
        case 76:  p.ampD = vf * 3000.0f; break;
        case 77:  p.ampS = vf;           break;
        case 72:  p.ampR = vf * 5000.0f; break;

        // LFO
        case 76 + 4: /* placeholder, avoid collision */ break;
        case 3:   p.lfoRate  = 0.05f + vf * 19.95f;    break;
        case 9:   p.lfoDepth = vf;                     break;

        // Chorus mode quick-select (value bucket): 0=OFF,1=I,2=II
        case 93:
            if (val < 43)       p.chorusMode = 0;
            else if (val < 86)  p.chorusMode = 1;
            else                p.chorusMode = 2;
            // Re-seed chorus defaults when switching modes via MIDI
            if (p.chorusMode == 1) { p.chorusRate = 0.513f; p.chorusDepth = 22.0f; }
            else if (p.chorusMode == 2) { p.chorusRate = 0.863f; p.chorusDepth = 36.0f; }
            break;

        case 120: // All sound off
        case 123: // All notes off
            synth.allNotesOff();
            return;

        default:
            return;
    }
    synth.applyPatch(p);
}

static void onPitchBend(byte /*ch*/, int bend) {
    // bend is -8192..+8191
    float norm = (float)bend / 8192.0f;
    if (norm > 1.0f)  norm = 1.0f;
    if (norm < -1.0f) norm = -1.0f;
    synth.setPitchBend(norm * PITCH_BEND_RANGE_SEMI);
}

void MidiHandler::begin() {
    dinMidi.begin(MIDI_CHANNEL_OMNI);
    dinMidi.setHandleNoteOn(onNoteOn);
    dinMidi.setHandleNoteOff(onNoteOff);
    dinMidi.setHandleControlChange(onCC);
    dinMidi.setHandlePitchBend(onPitchBend);

    usbMIDI.setHandleNoteOn(onNoteOn);
    usbMIDI.setHandleNoteOff(onNoteOff);
    usbMIDI.setHandleControlChange(onCC);
    usbMIDI.setHandlePitchChange(onPitchBend);  // usbMIDI uses a slightly different name
}

void MidiHandler::update() {
    dinMidi.read();
    while (usbMIDI.read()) {}
}
