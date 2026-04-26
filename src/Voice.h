#pragma once
#include <Audio.h>

class Voice {
public:
    Voice();
    void init(AudioSynthWaveform* sawOsc,
              AudioSynthWaveform* pulseOsc,
              AudioSynthWaveform* subOsc,
              AudioMixer4*        oscMix,
              AudioFilterStateVariable* filter,
              AudioEffectEnvelope* ampEnv,
              AudioEffectEnvelope* filtEnv);

    void noteOn(uint8_t note, uint8_t velocity);
    void noteOff();
    bool     isActive()     const { return active; }
    bool     isFullyDone()  const;
    uint8_t  getNote()      const { return currentNote; }
    uint8_t  getVelocity()  const { return currentVelocity; }
    uint32_t getStartTime() const { return startTime; }

    // Parameter setters (set targets for smoothing)
    void setWaveMix(float sawLvl, float pulseLvl, float subLvl);
    void setPulseWidth(float pw);
    void setFilterCutoff(float hz);
    void setFilterResonance(float q);
    void setAmpEnv(float a, float d, float s, float r);
    void setFiltEnv(float a, float d, float s, float r);
    void setEnvAmount(float amt) { envAmount = amt; }
    void setGlideMs(float ms)    { glideMs = ms; }

    // Control-rate tick: smoother + glide
    void tickSmoothing();

    // Applies modulation to the audio graph (main-loop context)
    void applyModulation(float lfoPitchSemi,
                         float lfoPWOffset,
                         float lfoFilterMul,
                         float filtEnvValue);

private:
    AudioSynthWaveform*       saw_ = nullptr;
    AudioSynthWaveform*       pulse_ = nullptr;
    AudioSynthWaveform*       sub_ = nullptr;
    AudioMixer4*              mix_ = nullptr;
    AudioFilterStateVariable* filter_ = nullptr;
    AudioEffectEnvelope*      ampEnv_ = nullptr;
    AudioEffectEnvelope*      filtEnv_ = nullptr;

    bool     active = false;
    bool     releasing = false;
    uint8_t  currentNote = 0;
    uint8_t  currentVelocity = 100;
    uint32_t startTime = 0;

    // Base values (targets for smoothing)
    float basePW          = 0.5f;
    float baseCutoff      = 1200.0f;
    float baseCutoffTarget= 1200.0f;
    float basePWTarget    = 0.5f;
    float smoothCutoff    = 1200.0f;
    float smoothPW        = 0.5f;

    // Pitch + glide
    float baseFreq    = 110.0f;
    float glideFreq   = 110.0f;
    float glideTarget = 110.0f;
    float glideMs     = 0.0f;

    float envAmount   = 0.5f;

    // Filter envelope estimator
    float filtA = 5, filtD = 120, filtS = 0.4f, filtR = 250;
    uint32_t noteOnMs = 0;
    bool    released = false;
    uint32_t releaseMs = 0;
    float   releaseStartVal = 0.0f;
    float   estimateFiltEnv();
};
