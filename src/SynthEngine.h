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

enum VelDest : uint8_t {
    VEL_DEST_OFF    = 0,
    VEL_DEST_VCA    = 1,
    VEL_DEST_CUTOFF = 2,
    VEL_DEST_LFO    = 3
};

enum class ParamId : uint8_t {
    SawLevel, PulseLevel, SubLevel, PulseWidth,
    Cutoff, Resonance, EnvAmount, HpfCutoff,
    AmpA, AmpD, AmpS, AmpR,
    FltA, FltD, FltS, FltR,
    LfoRate, LfoDepth,
    ChorusRate, ChorusDepth,
    VelAmount, GlideMs,
    Drive           // <-- ADD THIS
};

struct PatchData {
    char  name[17] = "INIT PATCH";

    // Oscillator
    float sawLevel   = 0.5f;
    float pulseLevel = 0.5f;
    float subLevel   = 0.3f;
    float pulseWidth = 0.5f;

    // Filter
    float cutoff     = 2000.0f;
    float resonance  = 0.7f;
    float envAmount  = 0.5f;
    float hpfCutoff  = 20.0f;

    // Envelopes
    float ampA = 5,  ampD = 50,  ampS = 0.8f, ampR = 250;
    float fltA = 5,  fltD = 120, fltS = 0.4f, fltR = 250;

    // LFO
    float   lfoRate   = 4.0f;
    float   lfoDepth  = 0.0f;
    uint8_t lfoDest   = LFO_DEST_OFF;
    uint8_t lfoShape  = 0;

    // Chorus
    uint8_t chorusMode  = 0;
    float   chorusRate  = 0.513f;
    float   chorusDepth = 22.0f;

    // Drive
    float drive = 1.0f;   // 1 = clean; up to 8 for heavy saturation

    // Velocity
    uint8_t velDest   = VEL_DEST_VCA;
    float   velAmount = 0.8f;

    // Glide / portamento (ms)
    float   glideMs   = 0.0f;

    // MIDI routing
    uint8_t midiChannel = 0;        // 0 = OMNI, 1..16 = specific channel
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

    // Fast-path single-parameter update (used by UI sliders)
    void setParam(ParamId id, float value);

    // Control-rate ISR entry (stashes mod values)
    void controlTick();

    // Main-loop consumer (applies pending mod to voices)
    void update();

    // Performance modulation
    void setPitchBend(float semitones);
    void setModWheel(float amount);
    float getPitchBendSemi() const { return pitchBendSemi; }

    // LFO access
    float currentLfo() const { return lfoValue; }
    float currentLfoPitchSemi() const;
    float currentLfoPWOffset()  const;
    float currentLfoFilterMul() const;

    int  voiceCount() const { return MAX_VOICES; }
    bool isVoiceSounding(int i) const {
        return (i >= 0 && i < MAX_VOICES) ? voices[i].isSounding() : false;
    }
    enum class VoiceState : uint8_t {
    Idle      = 0,
    Held      = 1,
    Releasing = 2
    };

    VoiceState voiceState(int i) const {
        if (i < 0 || i >= MAX_VOICES) return VoiceState::Idle;
        if (voices[i].isHeld())      return VoiceState::Held;
        if (voices[i].isReleasing()) return VoiceState::Releasing;
        return VoiceState::Idle;
    }

    float cpuUsagePercent();

    float peakLevelL();   // returns 0..1, consumes the peak
    float peakLevelR();

    void setSustain(bool on);
    bool sustain() const { return sustainOn; }
    bool matchesChannel(uint8_t incomingChannel) const;

private:
    int  findFreeVoice(uint8_t note);
    void applyChorus();
    float effectiveLfoDepth() const {
        float d = currentPatch.lfoDepth + (1.0f - currentPatch.lfoDepth) * modWheelAmt;
        if (d > 1.0f) d = 1.0f;
        return d;
    }

    Voice voices[MAX_VOICES];
    PatchData currentPatch;

    // LFO state
    float lfoPhase = 0.0f;
    float lfoValue = 0.0f;

    // ISR -> main-loop handoff
    volatile float modPitchSemi = 0.0f;
    volatile float modPWOff     = 0.0f;
    volatile float modFiltMul   = 1.0f;
    volatile bool  modDirty     = false;

    // Performance mod
    float pitchBendSemi = 0.0f;
    float modWheelAmt   = 0.0f;

    bool    sustainOn = false;
    uint8_t sustainedNotes[128] = {0};   // bitmap of notes held by sustain
};

extern SynthEngine synth;