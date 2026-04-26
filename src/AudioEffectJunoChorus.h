#pragma once
#include <Audio.h>
#include <AudioStream.h>

class AudioEffectJunoChorus : public AudioStream {
public:
    AudioEffectJunoChorus();

    static constexpr int BUFSIZE = 2048;

    void setMode(uint8_t mode);
    void setRate(float hz);
    void setDepth(float smp);

    virtual void update(void) override;

private:
    audio_block_t* inputQueueArray[1];

    int16_t bufL[BUFSIZE];
    int16_t bufR[BUFSIZE];
    int     widx = 0;

    float lfoPhase = 0.0f;

    float lfoSmoothL = 0.0f;
    float lfoSmoothR = 0.0f;

    float preLpL = 0.0f;
    float preLpR = 0.0f;

    float outLpL = 0.0f;
    float outLpR = 0.0f;

    float fbL = 0.0f;
    float fbR = 0.0f;

    volatile bool  active   = false;
    volatile float lfoRate  = 0.513f;
    volatile float depthSmp = 22.0f;
    volatile float baseSmp  = 220.0f;

    static constexpr float LFO_SMOOTH_COEFF  = 0.05f;
    static constexpr float PRE_HICUT_COEFF   = 0.35f;
    static constexpr float OUT_HICUT_COEFF   = 0.28f;
    static constexpr float FEEDBACK_AMOUNT   = 0.12f;

    static inline int16_t satI16(int32_t v) {
        if (v >  32767) return  32767;
        if (v < -32768) return -32768;
        return (int16_t)v;
    }
};
