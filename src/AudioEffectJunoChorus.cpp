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

void AudioEffectJunoChorus::setRate(float hz) {
    if (hz < 0.05f) hz = 0.05f;
    if (hz > 8.0f)  hz = 8.0f;
    lfoRate = hz;
}

void AudioEffectJunoChorus::setDepth(float smp) {
    if (smp < 0.0f)  smp = 0.0f;
    if (smp > 80.0f) smp = 80.0f;
    depthSmp = smp;
}

void AudioEffectJunoChorus::update(void) {
    audio_block_t* in   = receiveReadOnly(0);
    audio_block_t* outL = allocate();
    audio_block_t* outR = allocate();

    if (!outL || !outR) {
        if (in)   release(in);
        if (outL) release(outL);
        if (outR) release(outR);
        return;
    }

    // Bypass path: emit silence so the dry signal (mixed externally) is all you hear.
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

    const float sr       = AUDIO_SAMPLE_RATE_EXACT;
    const float phaseInc = lfoRate / sr;
    const float twoPi    = 2.0f * (float)M_PI;

    for (int i = 0; i < AUDIO_BLOCK_SAMPLES; i++) {
        // ------------------------------------------------------------
        // 1) Pre-chorus high-cut (one-pole LPF on the INPUT, per side)
        //    Emulates the limited top-end of a BBD input stage.
        // ------------------------------------------------------------
        float xf = (float)in->data[i];
        preLpL += (xf - preLpL) * PRE_HICUT_COEFF;
        preLpR += (xf - preLpR) * PRE_HICUT_COEFF;

        // ------------------------------------------------------------
        // 2) Write to delay lines WITH a small feedback of the prior
        //    wet-out sample. This adds the gentle BBD coloration.
        // ------------------------------------------------------------
        float inL = preLpL + fbL * FEEDBACK_AMOUNT;
        float inR = preLpR + fbR * FEEDBACK_AMOUNT;
        bufL[widx] = satI16((int32_t)inL);
        bufR[widx] = satI16((int32_t)inR);

        // ------------------------------------------------------------
        // 3) LFO: sine, opposite phase on the two taps.
        //    Smoothed with a one-pole filter for extra silk.
        // ------------------------------------------------------------
        float pL = lfoPhase;
        float pR = lfoPhase + 0.5f;
        if (pR >= 1.0f) pR -= 1.0f;

        float rawL = sinf(pL * twoPi);
        float rawR = sinf(pR * twoPi);

        lfoSmoothL += (rawL - lfoSmoothL) * LFO_SMOOTH_COEFF;
        lfoSmoothR += (rawR - lfoSmoothR) * LFO_SMOOTH_COEFF;

        float dL = baseSmp + depthSmp * lfoSmoothL;
        float dR = baseSmp + depthSmp * lfoSmoothR;

        // ------------------------------------------------------------
        // 4) Read fractional-delayed samples from the lines.
        // ------------------------------------------------------------
        float rL = (float)widx - dL;
        float rR = (float)widx - dR;
        while (rL < 0) rL += BUFSIZE;
        while (rR < 0) rR += BUFSIZE;

        int   iL  = (int)rL;
        float fL  = rL - iL;
        int   iL2 = (iL + 1) % BUFSIZE;

        int   iR  = (int)rR;
        float fR  = rR - iR;
        int   iR2 = (iR + 1) % BUFSIZE;

        float wetL = (1.0f - fL) * bufL[iL] + fL * bufL[iL2];
        float wetR = (1.0f - fR) * bufR[iR] + fR * bufR[iR2];

        // ------------------------------------------------------------
        // 5) Output high-cut (one-pole LPF on the WET path, per side).
        //    Emulates the BBD output's reconstruction filtering.
        // ------------------------------------------------------------
        outLpL += (wetL - outLpL) * OUT_HICUT_COEFF;
        outLpR += (wetR - outLpR) * OUT_HICUT_COEFF;

        // Stash for next sample's feedback
        fbL = outLpL;
        fbR = outLpR;

        outL->data[i] = satI16((int32_t)outLpL);
        outR->data[i] = satI16((int32_t)outLpR);

        // ------------------------------------------------------------
        // 6) Advance pointers
        // ------------------------------------------------------------
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
