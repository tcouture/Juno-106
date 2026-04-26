#pragma once
#include <Audio.h>
#include <AudioStream.h>

// Stereo Juno-style chorus:
//   - Two modulated delay lines (one per output channel)
//   - Opposite-phase LFOs so the two sides move in anti-phase
//   - Modes: OFF / I (slow, ~0.5 Hz) / II (faster, ~0.83 Hz, deeper)
// Input: one mono channel (index 0)
// Outputs: stereo wet (L=0, R=1) — mix with dry externally.
class AudioEffectJunoChorus : public AudioStream {
public:
    AudioEffectJunoChorus();

    // Delay buffer length in samples per side (must be >= max delay + some).
    // ~40 ms @ 44.1 kHz ~= 1764 samples; 2048 is comfortable.
    static constexpr int BUFSIZE = 2048;

    // mode: 0=off, 1=Chorus I, 2=Chorus II
    void setMode(uint8_t mode);

    virtual void update(void) override;

    void setRate(float hz);
    void setDepth(float smp);

private:
    audio_block_t* inputQueueArray[1];

    // Delay buffers (per side)
    int16_t bufL[BUFSIZE];
    int16_t bufR[BUFSIZE];
    int     widx = 0;

    // LFO phase (0..1) shared; second tap uses +0.5 offset
    float   lfoPhase = 0.0f;

    // Current parameters (set by setMode)
    volatile bool    active   = false;
    volatile float   lfoRate  = 0.50f;    // Hz
    volatile float   depthSmp = 30.0f;    // peak modulation amount (samples)
    volatile float   baseSmp  = 220.0f;   // center delay (samples)

    static inline int16_t satI16(int32_t v) {
        if (v >  32767) return  32767;
        if (v < -32768) return -32768;
        return (int16_t)v;
    }
};

