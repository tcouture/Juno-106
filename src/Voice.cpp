#include "Voice.h"
#include <math.h>

static float midiToFreq(uint8_t note) {
    return 440.0f * powf(2.0f, (note - 69) / 12.0f);
}

Voice::Voice() {}

void Voice::init(AudioSynthWaveform* sawOsc,
                 AudioSynthWaveform* pulseOsc,
                 AudioSynthWaveform* subOsc,
                 AudioMixer4* oscMix,
                 AudioFilterStateVariable* filter,
                 AudioEffectEnvelope* ampEnv,
                 AudioEffectEnvelope* filtEnv) {
    saw_ = sawOsc; pulse_ = pulseOsc; sub_ = subOsc;
    mix_ = oscMix; filter_ = filter;
    ampEnv_ = ampEnv; filtEnv_ = filtEnv;

    saw_->begin(0.0f, 110.0f, WAVEFORM_BANDLIMIT_SAWTOOTH);
    pulse_->begin(0.0f, 110.0f, WAVEFORM_BANDLIMIT_PULSE);
    pulse_->pulseWidth(0.5f);
    sub_->begin(0.0f, 55.0f, WAVEFORM_BANDLIMIT_SQUARE);

    mix_->gain(0, 0.5f); mix_->gain(1, 0.5f);
    mix_->gain(2, 0.3f); mix_->gain(3, 0.0f);

    filter_->frequency(1200);
    filter_->resonance(0.7f);

    ampEnv_->attack(5); ampEnv_->decay(50);
    ampEnv_->sustain(0.8f); ampEnv_->release(250);

    filtEnv_->attack(5); filtEnv_->decay(120);
    filtEnv_->sustain(0.4f); filtEnv_->release(250);
}

bool Voice::isFullyDone() const {
    if (!releasing) return false;
    return (millis() - releaseMs) >= (uint32_t)(filtR + 20);
}

void Voice::noteOn(uint8_t note, uint8_t velocity) {
    currentNote = note;
    currentVelocity = velocity;
    active = true;
    releasing = false;
    released = false;
    startTime = millis();
    noteOnMs = startTime;

    float newFreq = midiToFreq(note);
    baseFreq = newFreq;

    if (glideMs <= 0.0f) {
        glideFreq = newFreq;
    }
    glideTarget = newFreq;

    float amp = velocity / 127.0f;
    saw_->amplitude(0.6f * amp);
    pulse_->amplitude(0.6f * amp);
    sub_->amplitude(0.5f * amp);

    ampEnv_->noteOn();
    filtEnv_->noteOn();
}

void Voice::noteOff() {
    ampEnv_->noteOff();
    filtEnv_->noteOff();
    released = true;
    releasing = true;
    releaseMs = millis();
    releaseStartVal = estimateFiltEnv();
    active = false;

    // Reset cutoff smoothing so next allocation starts from patch base
    baseCutoffTarget = baseCutoff;
}

void Voice::setWaveMix(float s, float p, float u) {
    mix_->gain(0, s); mix_->gain(1, p); mix_->gain(2, u);
}
void Voice::setPulseWidth(float pw)    { basePW = pw; basePWTarget = pw; }
void Voice::setFilterCutoff(float hz)  { baseCutoff = hz; baseCutoffTarget = hz; }
void Voice::setFilterResonance(float q){ filter_->resonance(q); }

void Voice::setAmpEnv(float a, float d, float s, float r) {
    ampEnv_->attack(a); ampEnv_->decay(d); ampEnv_->sustain(s); ampEnv_->release(r);
}
void Voice::setFiltEnv(float a, float d, float s, float r) {
    filtA=a; filtD=d; filtS=s; filtR=r;
    filtEnv_->attack(a); filtEnv_->decay(d); filtEnv_->sustain(s); filtEnv_->release(r);
}

float Voice::estimateFiltEnv() {
    if (!active && !released) return 0.0f;
    uint32_t now = millis();
    if (released) {
        float t = (now - releaseMs);
        if (t >= filtR) return 0.0f;
        return releaseStartVal * (1.0f - t / filtR);
    }
    float t = (float)(now - noteOnMs);
    if (t < filtA) return t / filtA;
    t -= filtA;
    if (t < filtD) {
        float k = t / filtD;
        return 1.0f + (filtS - 1.0f) * k;
    }
    return filtS;
}

void Voice::tickSmoothing() {
    // ~5 ms time constant at 1 kHz control rate
    const float cutCoeff = 0.94f;
    const float pwCoeff  = 0.90f;
    smoothCutoff = smoothCutoff * cutCoeff + baseCutoffTarget * (1.0f - cutCoeff);
    smoothPW     = smoothPW     * pwCoeff  + basePWTarget     * (1.0f - pwCoeff);

    // Glide: exponential ramp of glideFreq -> glideTarget
    if (glideMs > 0.5f) {
        // ~5x time-constant to reach target; compute per-tick coefficient
        float tauTicks = glideMs;                 // 1 tick = 1 ms
        float k = expf(-1.0f / tauTicks);         // per-tick blend
        glideFreq = glideTarget + (glideFreq - glideTarget) * k;
    } else {
        glideFreq = glideTarget;
    }
}

void Voice::applyModulation(float lfoPitchSemi, float lfoPWOffset,
                            float lfoFilterMul, float /*externFiltEnv*/) {
    if (!active && !released) return;

    float f = glideFreq * powf(2.0f, lfoPitchSemi / 12.0f);
    saw_->frequency(f);
    pulse_->frequency(f);
    sub_->frequency(f * 0.5f);

    float pw = smoothPW + lfoPWOffset;
    if (pw < 0.05f) pw = 0.05f;
    if (pw > 0.95f) pw = 0.95f;
    pulse_->pulseWidth(pw);

    float env = estimateFiltEnv();
    float cutoff = smoothCutoff * lfoFilterMul * (1.0f + envAmount * 4.0f * env);
    if (cutoff < 20.0f)    cutoff = 20.0f;
    if (cutoff > 15000.0f) cutoff = 15000.0f;
    filter_->frequency(cutoff);
}
