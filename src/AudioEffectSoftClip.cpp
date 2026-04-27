#include "AudioEffectSoftClip.h"
#include <math.h>

void AudioEffectSoftClip::update(void) {
    audio_block_t* in = receiveWritable(0);
    if (!in) return;

    const float d  = drive;
    const float og = outGain;
    const float kInv = 1.0f / 32768.0f;
    const float kOut = 32767.0f;

    for (int i = 0; i < AUDIO_BLOCK_SAMPLES; i++) {
        float x = (float)in->data[i] * kInv;
        // tanh-style soft clip
        float y = tanhf(x * d) * og;
        int32_t s = (int32_t)(y * kOut);
        if (s >  32767) s =  32767;
        if (s < -32768) s = -32768;
        in->data[i] = (int16_t)s;
    }

    transmit(in);
    release(in);
}

