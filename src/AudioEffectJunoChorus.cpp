#include "AudioEffectJunoChorus.h"
#include <math.h>

AudioEffectJunoChorus::AudioEffectJunoChorus()
    : AudioStream(1, inputQueueArray)
{
    memset(bufL, 0, sizeof(bufL));
    memset(bufR, 0, sizeof(bufR));
}

void AudioEffectJunoChorus::setMode(uint8_t mode) {
    switch (mode) {
        case 1: // Chorus I: slow & mellow
            lfoRate  = 0.513f;
            depthSmp = 22.0f;
            baseSmp  = 220.0f;
            active   = true;
            break;
        case 2: // Chorus II: faster & deeper
            lfoRate  = 0.863f;
            depthSmp = 36.0f;
            baseSmp  = 220.0f;
            active   = true;
            break;
        case 0:
        default:
            active = false;
            break;
    }
}

void AudioEffectJunoChorus::update(void) {
    audio_block_t* in = receiveReadOnly(0);
    audio_block_t* outL = allocate();
    audio_block_t* outR = allocate();

    if (!outL || !outR) {
        if (in) release(in);
        if (outL) release(outL);
        if (outR) release(outR);
        return;
    }

    // If disabled, emit silence (dry is mixed externally)
    if (!active || !in) {
        memset(outL->data, 0, sizeof(outL->data));
        memset(outR->data, 0, sizeof(outR->data));
        transmit(outL, 0);
        transmit(outR, 1);
        release(outL);
        release(outR);
        if (in) release(in);
        return;
    }

    const float sr = AUDIO_SAMPLE_RATE_EXACT;
    const float phaseInc = lfoRate / sr;

    for (int i = 0; i < AUDIO_BLOCK_SAMPLES; i++) {
        // Write input to both delay lines
        int16_t x = in->data[i];
        bufL[widx] = x;
        bufR[widx] = x;

        // Two LFO phases 180° apart
        float pL = lfoPhase;
        float pR = lfoPhase + 0.5f;
        if (pR >= 1.0f) pR -= 1.0f;

        // Triangle LFO in [-1, 1]
        auto tri = [](float p) {
            float x = p * 4.0f;
            if      (x < 1.0f) return x;
            else if (x < 3.0f) return 2.0f - x;
            else               return x - 4.0f;
        };
        float lfoL = tri(pL);
        float lfoR = tri(pR);

        float dL = baseSmp + depthSmp * lfoL;
        float dR = baseSmp + depthSmp * lfoR;

        // Read pointers with fractional delay
        float rL = (float)widx - dL;
        float rR = (float)widx - dR;
        while (rL < 0) rL += BUFSIZE;
        while (rR < 0) rR += BUFSIZE;

        int   iL = (int)rL;
        float fL = rL - iL;
        int   iL2 = (iL + 1) % BUFSIZE;
        int   iR = (int)rR;
        float fR = rR - iR;
        int   iR2 = (iR + 1) % BUFSIZE;

        int16_t sL = (int16_t)((1.0f - fL) * bufL[iL] + fL * bufL[iL2]);
        int16_t sR = (int16_t)((1.0f - fR) * bufR[iR] + fR * bufR[iR2]);

        outL->data[i] = sL;
        outR->data[i] = sR;

        // Advance
        widx++;
        if (widx >= BUFSIZE) widx = 0;
        lfoPhase += phaseInc;
        if (lfoPhase >= 1.0f) lfoPhase -= 1.0f;
    }

    transmit(outL, 0);
    transmit(outR, 1);
    release(outL);
    release(outR);
    release(in);
}

void AudioEffectJunoChorus::setRate(float hz) {
    if (hz < 0.05f) hz = 0.05f;
    if (hz > 8.0f)  hz = 8.0f;
    lfoRate = hz;
}

void AudioEffectJunoChorus::setDepth(float smp) {
    if (smp < 0.0f)  smp = 0.0f;
    if (smp > 80.0f) smp = 80.0f;  // stay well within BUFSIZE margin
    depthSmp = smp;
}


