#include "MidiHandler.h"
#include "SynthEngine.h"
#include "Arpeggiator.h"
#include "MidiActivity.h"
#include "Config.h"

MIDI_CREATE_INSTANCE(HardwareSerial, MIDI_SERIAL, dinMidi);
MidiHandler midiHandler;

static constexpr float PITCH_BEND_RANGE_SEMI = 2.0f;

// ---- Core handlers (source-agnostic) ----
static void handleNoteOnCore(byte, byte note, byte vel) {
    if (vel == 0) { arp.noteOff(note); return; }
    arp.noteOn(note, vel);
}
static void handleNoteOffCore(byte, byte note, byte) {
    arp.noteOff(note);
}
static void handleCCCore(byte, byte cc, byte val) {
    PatchData& p = synth.patch();
    float vf = val / 127.0f;

    switch (cc) {
        case 1:   synth.setModWheel(vf); return;
        case 20:  synth.setParam(ParamId::SawLevel,   vf); return;
        case 21:  synth.setParam(ParamId::PulseLevel, vf); return;
        case 22:  synth.setParam(ParamId::SubLevel,   vf); return;
        case 70:  synth.setParam(ParamId::PulseWidth, 0.05f + vf * 0.9f); return;
        case 74:  synth.setParam(ParamId::Cutoff,    40.0f + vf * 7960.0f); return;
        case 71:  synth.setParam(ParamId::Resonance, 0.7f  + vf * 4.3f);    return;
        case 79:  synth.setParam(ParamId::EnvAmount, vf); return;
        case 75:  synth.setParam(ParamId::HpfCutoff, 20.0f + vf * 980.0f); return;
        case 73:  synth.setParam(ParamId::AmpA, vf * 3000.0f); return;
        case 76:  synth.setParam(ParamId::AmpD, vf * 3000.0f); return;
        case 77:  synth.setParam(ParamId::AmpS, vf);           return;
        case 72:  synth.setParam(ParamId::AmpR, vf * 5000.0f); return;
        case 3:   synth.setParam(ParamId::LfoRate,  0.05f + vf * 19.95f); return;
        case 9:   synth.setParam(ParamId::LfoDepth, vf); return;
        case 5:   synth.setParam(ParamId::GlideMs, vf * 1000.0f); return;
        case 93:
            if (val < 43)       p.chorusMode = 0;
            else if (val < 86)  p.chorusMode = 1;
            else                p.chorusMode = 2;
            if (p.chorusMode == 1) { p.chorusRate = 0.513f; p.chorusDepth = 22.0f; }
            else if (p.chorusMode == 2) { p.chorusRate = 0.863f; p.chorusDepth = 36.0f; }
            synth.applyPatch(p);
            return;
        case 120:
        case 123:
            synth.allNotesOff();
            return;
        default: return;
    }
}
static void handlePitchBendCore(byte, int bend) {
    float norm = (float)bend / 8192.0f;
    if (norm > 1.0f)  norm = 1.0f;
    if (norm < -1.0f) norm = -1.0f;
    synth.setPitchBend(norm * PITCH_BEND_RANGE_SEMI);
}

// ---- DIN wrappers (ping DIN activity) ----
static void dinNoteOn (byte ch, byte n, byte v) { midiActivity.ping(MidiSource::Din); handleNoteOnCore (ch, n, v); }
static void dinNoteOff(byte ch, byte n, byte v) { midiActivity.ping(MidiSource::Din); handleNoteOffCore(ch, n, v); }
static void dinCC     (byte ch, byte c, byte v) { midiActivity.ping(MidiSource::Din); handleCCCore     (ch, c, v); }
static void dinBend   (byte ch, int  b)         { midiActivity.ping(MidiSource::Din); handlePitchBendCore(ch, b); }

// ---- USB-device wrappers (ping UsbDevice activity) ----
static void usbNoteOn (byte ch, byte n, byte v) { midiActivity.ping(MidiSource::UsbDevice); handleNoteOnCore (ch, n, v); }
static void usbNoteOff(byte ch, byte n, byte v) { midiActivity.ping(MidiSource::UsbDevice); handleNoteOffCore(ch, n, v); }
static void usbCC     (byte ch, byte c, byte v) { midiActivity.ping(MidiSource::UsbDevice); handleCCCore     (ch, c, v); }
static void usbBend   (byte ch, int  b)         { midiActivity.ping(MidiSource::UsbDevice); handlePitchBendCore(ch, b); }

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
