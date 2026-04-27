#pragma once
#include <Audio.h>
#include <AudioStream.h>

// Simple tanh-style soft clipper with gain-before and gain-after.
class AudioEffectSoftClip : public AudioStream {
public:
    AudioEffectSoftClip() : AudioStream(1, inputQueueArray) {}

    // drive: 1.0 = clean, higher = more saturation. Sensible range 1..8.
    void setDrive(float d) {
        if (d < 1.0f) d = 1.0f;
        if (d > 16.0f) d = 16.0f;
        drive = d;
        // Compensate output so perceived level stays similar as drive increases
        outGain = 1.0f / sqrtf(d);
    }

    virtual void update(void) override;

private:
    audio_block_t* inputQueueArray[1];
    volatile float drive = 1.0f;
    volatile float outGain = 1.0f;
};

