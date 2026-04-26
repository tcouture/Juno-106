#include "SynthEngine.h"
#include "AudioEffectJunoChorus.h"
#include <vector>
#include <math.h>

// ---- Per-voice audio graph ----
AudioSynthWaveform        saw[MAX_VOICES];
AudioSynthWaveform        pulse[MAX_VOICES];
AudioSynthWaveform        sub[MAX_VOICES];
AudioMixer4               oscMix[MAX_VOICES];
AudioFilterStateVariable  filt[MAX_VOICES];
AudioEffectEnvelope       ampEnv[MAX_VOICES];
AudioEffectEnvelope       fltEnv[MAX_VOICES];

AudioMixer4 subMix1, subMix2, subMix3, subMix4;
AudioMixer4 preHPF;
AudioFilterStateVariable hpf;

AudioEffectJunoChorus junoChorus;

AudioMixer4 mixL, mixR;
AudioOutputI2S i2sOut;
AudioControlSGTL5000 codec;

static std::vector<AudioConnection*> cables;

static IntervalTimer ctrlTimer;
static void controlIsr();

SynthEngine synth;

SynthEngine::SynthEngine() {}

void SynthEngine::begin() {
    AudioMemory(200);
    codec.enable();
    codec.volume(0.6f);

    for (int i = 0; i < MAX_VOICES; i++) {
        cables.push_back(new AudioConnection(saw[i],   0, oscMix[i], 0));
        cables.push_back(new AudioConnection(pulse[i], 0, oscMix[i], 1));
        cables.push_back(new AudioConnection(sub[i],   0, oscMix[i], 2));
        cables.push_back(new AudioConnection(oscMix[i],0, filt[i],   0));
        cables.push_back(new AudioConnection(filt[i],  0, ampEnv[i],0));

        AudioMixer4* target = (i<4)?&subMix1:(i<8)?&subMix2:(i<12)?&subMix3:&subMix4;
        cables.push_back(new AudioConnection(ampEnv[i], 0, *target, i%4));

        voices[i].init(&saw[i], &pulse[i], &sub[i],
                       &oscMix[i], &filt[i], &ampEnv[i], &fltEnv[i]);
    }

    cables.push_back(new AudioConnection(subMix1, 0, preHPF, 0));
    cables.push_back(new AudioConnection(subMix2, 0, preHPF, 1));
    cables.push_back(new AudioConnection(subMix3, 0, preHPF, 2));
    cables.push_back(new AudioConnection(subMix4, 0, preHPF, 3));

    cables.push_back(new AudioConnection(preHPF, 0, hpf, 0));

    cables.push_back(new AudioConnection(hpf,        2, junoChorus, 0));
    cables.push_back(new AudioConnection(hpf,        2, mixL, 0));
    cables.push_back(new AudioConnection(junoChorus, 0, mixL, 1));
    cables.push_back(new AudioConnection(hpf,        2, mixR, 0));
    cables.push_back(new AudioConnection(junoChorus, 1, mixR, 1));

    cables.push_back(new AudioConnection(mixL, 0, i2sOut, 0));
    cables.push_back(new AudioConnection(mixR, 0, i2sOut, 1));

    for (int i = 0; i < 4; i++) {
        subMix1.gain(i, 0.25f); subMix2.gain(i, 0.25f);
        subMix3.gain(i, 0.25f); subMix4.gain(i, 0.25f);
        preHPF.gain(i,  0.9f);
    }
    mixL.gain(0, 1.0f); mixL.gain(1, 0.0f);
    mixR.gain(0, 1.0f); mixR.gain(1, 0.0f);

    hpf.frequency(currentPatch.hpfCutoff);
    hpf.resonance(0.707f);

    applyPatch(currentPatch);

    ctrlTimer.begin(controlIsr, 1000000UL / CONTROL_RATE_HZ);
}

static void controlIsr() { synth.controlTick(); }

int SynthEngine::findFreeVoice(uint8_t note) {
    for (int i = 0; i < MAX_VOICES; i++)
        if (voices[i].isActive() && voices[i].getNote() == note) return i;
    for (int i = 0; i < MAX_VOICES; i++)
        if (!voices[i].isActive() || voices[i].isFullyDone()) return i;
    int oldest = 0; uint32_t t = voices[0].getStartTime();
    for (int i = 1; i < MAX_VOICES; i++)
        if (voices[i].getStartTime() < t) { t = voices[i].getStartTime(); oldest = i; }
    return oldest;
}

void SynthEngine::noteOn(uint8_t note, uint8_t velocity) {
    if (velocity == 0) { noteOff(note); return; }
    int idx = findFreeVoice(note);
    Voice& v = voices[idx];

    float velNorm = velocity / 127.0f;
    float amt     = currentPatch.velAmount;

    // Velocity routing
    switch (currentPatch.velDest) {
        case VEL_DEST_CUTOFF: {
            float boost = 1.0f + velNorm * amt * 3.0f;
            v.setFilterCutoff(currentPatch.cutoff * boost);
            break;
        }
        case VEL_DEST_LFO:
            setModWheel(velNorm * amt);
            break;
        case VEL_DEST_VCA:
        case VEL_DEST_OFF:
        default:
            break;
    }

    uint8_t vToVoice = (currentPatch.velDest == VEL_DEST_VCA) ? velocity : 127;
    v.noteOn(note, vToVoice);
}

void SynthEngine::noteOff(uint8_t note) {
    for (int i = 0; i < MAX_VOICES; i++)
        if (voices[i].isActive() && voices[i].getNote() == note)
            voices[i].noteOff();
}
void SynthEngine::allNotesOff() {
    for (int i = 0; i < MAX_VOICES; i++) voices[i].noteOff();
}

void SynthEngine::applyChorus() {
    uint8_t mode = currentPatch.chorusMode;
    junoChorus.setMode(mode);
    if (mode != 0) {
        junoChorus.setRate(currentPatch.chorusRate);
        junoChorus.setDepth(currentPatch.chorusDepth);
    }
    float wet = (mode == 0) ? 0.0f : 0.7f;
    float dry = (mode == 0) ? 1.0f : 0.7f;
    mixL.gain(0, dry); mixL.gain(1, wet);
    mixR.gain(0, dry); mixR.gain(1, wet);
}

void SynthEngine::applyPatch(const PatchData& p) {
    currentPatch = p;
    for (int i = 0; i < MAX_VOICES; i++) {
        voices[i].setWaveMix(p.sawLevel, p.pulseLevel, p.subLevel);
        voices[i].setPulseWidth(p.pulseWidth);
        voices[i].setFilterCutoff(p.cutoff);
        voices[i].setFilterResonance(p.resonance);
        voices[i].setAmpEnv(p.ampA, p.ampD, p.ampS, p.ampR);
        voices[i].setFiltEnv(p.fltA, p.fltD, p.fltS, p.fltR);
        voices[i].setEnvAmount(p.envAmount);
        voices[i].setGlideMs(p.glideMs);
    }
    hpf.frequency(p.hpfCutoff);
    applyChorus();
}

void SynthEngine::setParam(ParamId id, float v) {
    PatchData& p = currentPatch;
    switch (id) {
        case ParamId::SawLevel:    p.sawLevel   = v; for (auto& vo : voices) vo.setWaveMix(p.sawLevel, p.pulseLevel, p.subLevel); break;
        case ParamId::PulseLevel:  p.pulseLevel = v; for (auto& vo : voices) vo.setWaveMix(p.sawLevel, p.pulseLevel, p.subLevel); break;
        case ParamId::SubLevel:    p.subLevel   = v; for (auto& vo : voices) vo.setWaveMix(p.sawLevel, p.pulseLevel, p.subLevel); break;
        case ParamId::PulseWidth:  p.pulseWidth = v; for (auto& vo : voices) vo.setPulseWidth(v); break;
        case ParamId::Cutoff:      p.cutoff     = v; for (auto& vo : voices) vo.setFilterCutoff(v); break;
        case ParamId::Resonance:   p.resonance  = v; for (auto& vo : voices) vo.setFilterResonance(v); break;
        case ParamId::EnvAmount:   p.envAmount  = v; for (auto& vo : voices) vo.setEnvAmount(v); break;
        case ParamId::HpfCutoff:   p.hpfCutoff  = v; hpf.frequency(v); break;
        case ParamId::AmpA:        p.ampA=v; for(auto& vo:voices) vo.setAmpEnv(p.ampA,p.ampD,p.ampS,p.ampR); break;
        case ParamId::AmpD:        p.ampD=v; for(auto& vo:voices) vo.setAmpEnv(p.ampA,p.ampD,p.ampS,p.ampR); break;
        case ParamId::AmpS:        p.ampS=v; for(auto& vo:voices) vo.setAmpEnv(p.ampA,p.ampD,p.ampS,p.ampR); break;
        case ParamId::AmpR:        p.ampR=v; for(auto& vo:voices) vo.setAmpEnv(p.ampA,p.ampD,p.ampS,p.ampR); break;
        case ParamId::FltA:        p.fltA=v; for(auto& vo:voices) vo.setFiltEnv(p.fltA,p.fltD,p.fltS,p.fltR); break;
        case ParamId::FltD:        p.fltD=v; for(auto& vo:voices) vo.setFiltEnv(p.fltA,p.fltD,p.fltS,p.fltR); break;
        case ParamId::FltS:        p.fltS=v; for(auto& vo:voices) vo.setFiltEnv(p.fltA,p.fltD,p.fltS,p.fltR); break;
        case ParamId::FltR:        p.fltR=v; for(auto& vo:voices) vo.setFiltEnv(p.fltA,p.fltD,p.fltS,p.fltR); break;
        case ParamId::LfoRate:     p.lfoRate  = v; break;
        case ParamId::LfoDepth:    p.lfoDepth = v; break;
        case ParamId::ChorusRate:  p.chorusRate  = v; junoChorus.setRate(v); break;
        case ParamId::ChorusDepth: p.chorusDepth = v; junoChorus.setDepth(v); break;
        case ParamId::VelAmount:   p.velAmount = v; break;
        case ParamId::GlideMs:     p.glideMs = v; for(auto& vo:voices) vo.setGlideMs(v); break;
    }
}

void SynthEngine::controlTick() {
    float dt = 1.0f / (float)CONTROL_RATE_HZ;
    lfoPhase += currentPatch.lfoRate * dt;
    if (lfoPhase >= 1.0f) lfoPhase -= 1.0f;

    switch (currentPatch.lfoShape) {
        case 1: lfoValue = sinf(lfoPhase * 2.0f * PI); break;
        case 2: lfoValue = (lfoPhase < 0.5f) ? 1.0f : -1.0f; break;
        case 3: lfoValue = 2.0f * lfoPhase - 1.0f; break;
        case 0:
        default: {
            float x = lfoPhase * 4.0f;
            if      (x < 1.0f) lfoValue = x;
            else if (x < 3.0f) lfoValue = 2.0f - x;
            else               lfoValue = x - 4.0f;
        }
    }

    float depth = effectiveLfoDepth();
    float pitchSemi = 0, pwOff = 0, filtMul = 1.0f;
    if (currentPatch.lfoDest == LFO_DEST_PITCH)
        pitchSemi = lfoValue * depth * 7.0f;
    else if (currentPatch.lfoDest == LFO_DEST_PW)
        pwOff = lfoValue * depth * 0.4f;
    else if (currentPatch.lfoDest == LFO_DEST_FILTER)
        filtMul = powf(2.0f, lfoValue * depth * 2.0f);

    pitchSemi += pitchBendSemi;

    // Smoothing + glide runs here at control rate
    for (int i = 0; i < MAX_VOICES; i++) voices[i].tickSmoothing();

    modPitchSemi = pitchSemi;
    modPWOff     = pwOff;
    modFiltMul   = filtMul;
    modDirty     = true;
}

void SynthEngine::update() {
    if (!modDirty) return;
    __disable_irq();
    float ps = modPitchSemi, pw = modPWOff, fm = modFiltMul;
    modDirty = false;
    __enable_irq();

    for (int i = 0; i < MAX_VOICES; i++)
        voices[i].applyModulation(ps, pw, fm, 0.0f);
}

void SynthEngine::setPitchBend(float semitones) { pitchBendSemi = semitones; }
void SynthEngine::setModWheel(float amount) {
    if (amount < 0) amount = 0;
    if (amount > 1) amount = 1;
    modWheelAmt = amount;
}

float SynthEngine::currentLfoPitchSemi() const {
    return (currentPatch.lfoDest == LFO_DEST_PITCH)
        ? lfoValue * currentPatch.lfoDepth * 7.0f : 0.0f;
}
float SynthEngine::currentLfoPWOffset() const {
    return (currentPatch.lfoDest == LFO_DEST_PW)
        ? lfoValue * currentPatch.lfoDepth * 0.4f : 0.0f;
}
float SynthEngine::currentLfoFilterMul() const {
    return (currentPatch.lfoDest == LFO_DEST_FILTER)
        ? powf(2.0f, lfoValue * currentPatch.lfoDepth * 2.0f) : 1.0f;
}