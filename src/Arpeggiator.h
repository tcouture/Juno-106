#pragma once
#include <Audio.h>
#include <AudioStream.h>

// Stereo Juno-style chorus:
//   * Two fractionally-delayed lines modulated by opposite-phase LFOs
//   * Sine LFO (corner-free) with optional one-pole smoothing
//   * Light feedback path -> subtle BBD-like coloration
//   * Pre-chorus high-cut + output high-cut to emulate BBD bandwidth
//
// Input : one mono channel (index 0)
// Output: stereo wet (L = 0, R = 1). Mix with dry externally.
class AudioEffectJunoChorus : public AudioStream {
public:
    AudioEffectJunoChorus();

    static constexpr int BUFSIZE = 2048;  // ~46 ms @ 44.1 kHz

    // mode: 0 = off, 1 = Chorus I, 2 = Chorus II
    void setMode(uint8_t mode);
    void setRate(float hz);
    void setDepth(float smp);

    virtual void update(void) override;

private:
    audio_block_t* inputQueueArray[1];

    // Delay lines
    int16_t bufL[BUFSIZE];
    int16_t bufR[BUFSIZE];
    int     widx = 0;

    // LFO phase (0..1); second tap uses phase + 0.5
    float lfoPhase = 0.0f;

    // Smoothed LFO values (one-pole LP to round out any ISR-rate noise)
    float lfoSmoothL = 0.0f;
    float lfoSmoothR = 0.0f;

    // Pre-chorus high-cut (one-pole LPF) state, per side
    float preLpL = 0.0f;
    float preLpR = 0.0f;

    // Output high-cut state, per side
    float outLpL = 0.0f;
    float outLpR = 0.0f;

    // Feedback memory (one sample of delayed wet, per side)
    float fbL = 0.0f;
    float fbR = 0.0f;

    // Parameters (set by setMode / setRate / setDepth)
    volatile bool  active   = false;
    volatile float lfoRate  = 0.513f;     // Hz
    volatile float depthSmp = 22.0f;      // peak modulation (samples)
    volatile float baseSmp  = 220.0f;     // center delay (samples)

    // Tone/coloration constants
    static constexpr float LFO_SMOOTH_COEFF  = 0.05f;  // ~3 ms
    static constexpr float PRE_HICUT_COEFF   = 0.35f;  // ~6 kHz @ 44.1 kHz
    static constexpr float OUT_HICUT_COEFF   = 0.28f;  // ~4.5 kHz
    static constexpr float FEEDBACK_AMOUNT   = 0.12f;  // subtle; keep < 0.2

    static inline int16_t satI16(int32_t v) {
        if (v >  32767) return  32767;
        if (v < -32768) return -32768;
        return (int16_t)v;
    }
};
