#pragma once
#include <Audio.h>
#include "Config.h"
#include "Voice.h"

enum LfoDest : uint8_t {
    LFO_DEST_OFF    = 0,
    LFO_DEST_PITCH  = 1,
    LFO_DEST_PW     = 2,
    LFO_DEST_FILTER = 3
};

struct PatchData {
    char  name[17] = "INIT PATCH";

    // Oscillator
    float sawLevel   = 0.5f;
    float pulseLevel = 0.5f;
    float subLevel   = 0.3f;
    float pulseWidth = 0.5f;

    // Filter
    float cutoff     = 2000.0f;    // Hz, base
    float resonance  = 0.7f;       // 0.7..5
    float envAmount  = 0.5f;       // 0..1 (filter env -> cutoff)
    float hpfCutoff  = 20.0f;      // Hz, simple HPF

    // Envelopes (ms, sustain 0..1)
    float ampA = 5,  ampD = 50,  ampS = 0.8f, ampR = 250;
    float fltA = 5,  fltD = 120, fltS = 0.4f, fltR = 250;

    // LFO
    float   lfoRate   = 4.0f;      // Hz
    float   lfoDepth  = 0.0f;      // 0..1
    uint8_t lfoDest   = LFO_DEST_OFF;
    uint8_t lfoShape  = 0;         // 0=triangle,1=sine,2=square,3=saw

    // Chorus
    uint8_t chorusMode = 0;        // 0=off, 1=I, 2=II
};

class SynthEngine {
public:
    SynthEngine();
    void begin();

    void noteOn(uint8_t note, uint8_t velocity);
    void noteOff(uint8_t note);
    void allNotesOff();

    void applyPatch(const PatchData& p);
    PatchData& patch() { return currentPatch; }

    // Control-rate tick (call at CONTROL_RATE_HZ from a timer)
    void controlTick();

    // LFO access (voices share one LFO)
    float currentLfo() const { return lfoValue; }
    float currentLfoPitchSemi() const;   // semitones
    float currentLfoPWOffset()  const;   // -0.4..+0.4
    float currentLfoFilterMul() const;   // multiplier applied to cutoff

private:
    int findFreeVoice(uint8_t note);
    void applyChorus();

    Voice voices[MAX_VOICES];
    PatchData currentPatch;

    // LFO state
    float lfoPhase = 0.0f;
    float lfoValue = 0.0f;          // -1..+1
};

extern SynthEngine synth;
